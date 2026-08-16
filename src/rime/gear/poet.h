//
// Copyright RIME Developers
// Distributed under the BSD License
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_POET_H_
#define RIME_POET_H_

#include <rime_api.h>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/contextual_translation.h>

namespace rime {

using WordGraph = map<int, map<int, DictEntryList>>;

class Grammar;
class Language;

// internal data structure used during the sentence making process.
// the output line of the algorithm is transformed to an<Sentence>.
struct Line {
  // be sure the pointer to predecessor Line object is stable. it works since
  // pointer to values stored in std::map and std::unordered_map are stable.
  const Line* predecessor;
  // shared ownership so a line may outlive the word graph it was built from,
  // which the incremental sentence decoding relies on.
  of<DictEntry> entry;
  size_t end_pos;
  double weight;
  size_t text_hash;  // for dedup

  static const Line kEmpty;

  bool empty() const { return !predecessor && !entry; }

  string last_word() const { return entry ? entry->text : string(); }

  struct Components {
    vector<const Line*> lines;

    Components(const Line* line) {
      for (const Line* cursor = line; !cursor->empty();
           cursor = cursor->predecessor) {
        lines.push_back(cursor);
      }
    }

    decltype(lines.crbegin()) begin() const { return lines.crbegin(); }
    decltype(lines.crend()) end() const { return lines.crend(); }
  };

  Components components() const { return Components(this); }

  string context() const {
    // look back 2 words
    return empty() ? string()
           : !predecessor || predecessor->empty()
               ? last_word()
               : predecessor->last_word() + last_word();
  }

  vector<size_t> word_lengths() const {
    vector<size_t> lengths;
    size_t last_end_pos = 0;
    for (const auto* c : components()) {
      lengths.push_back(c->end_pos - last_end_pos);
      last_end_pos = c->end_pos;
    }
    return lengths;
  }
};

// keep the best line candidate per last phrase
using LineCandidates = hash_map<string, Line>;

class RIME_DLL Poet {
 public:
  // Line "less", used to compare composed line of the same input range.
  using Compare = function<bool(const Line&, const Line&)>;

  static bool CompareWeight(const Line& one, const Line& other);
  static bool LeftAssociateCompare(const Line& one, const Line& other);

  Poet(const Language* language,
       Config* config,
       Compare compare = CompareWeight);
  ~Poet();

  // incremental decoding state, kept across key presses when the input is
  // only appended at the tail; carries the beam (or dp) state per end
  // position plus how far the graph was covered in the previous round
  struct IncrementalStates {
    map<int, LineCandidates> beam;
    map<int, Line> dp;
    size_t covered_len = 0;
    bool valid = false;
  };

  an<Sentence> MakeSentence(const WordGraph& graph,
                            size_t total_length,
                            const string& preceding_text);
  // incremental variant: continue from [states] beyond [covered_len];
  // the result is identical to a full decode when the graph only grew
  // at the tail, otherwise pass covered_len = 0 to force a fresh decode
  an<Sentence> MakeSentence(const WordGraph& graph,
                            size_t total_length,
                            const string& preceding_text,
                            size_t covered_len,
                            IncrementalStates* states);
  deque<an<Sentence>> MakeSentences(const WordGraph& graph,
                                    size_t total_length,
                                    const string& preceding_text,
                                    size_t count,
                                    double cutoff_threshold);

  template <class TranslatorT>
  an<Translation> ContextualWeighted(an<Translation> translation,
                                     const string& input,
                                     size_t start,
                                     TranslatorT* translator) {
    if (!translator->contextual_suggestions() || !grammar_) {
      return translation;
    }
    auto preceding_text = translator->GetPrecedingText(start);
    if (preceding_text.empty()) {
      return translation;
    }
    return New<ContextualTranslation>(translation, input, preceding_text,
                                      grammar_.get());
  }

 private:
  template <class Strategy>
  an<Sentence> MakeSentenceWithStrategy(
      const WordGraph& graph,
      size_t total_length,
      const string& preceding_text,
      size_t covered_len,
      map<int, typename Strategy::State>* states);

  const Language* language_;
  the<Grammar> grammar_;
  Compare compare_;
};

}  // namespace rime

#endif  // RIME_POET_H_
