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

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/contextual_translation.h>

namespace rime {

using WordGraph = map<int, map<int, DictEntryList>>;

class Grammar;
class Language;
struct Line;

class Poet {
 public:
  // Line "less", used to compare composed line of the same input range.
  using Compare = function<bool (const Line&, const Line&)>;

  static bool CompareWeight(const Line& one, const Line& other);
  static bool LeftAssociateCompare(const Line& one, const Line& other);

  Poet(const Language* language, Config* config,
       Compare compare = CompareWeight);
  ~Poet();

  an<Sentence> MakeSentence(const WordGraph& graph,
                            size_t total_length,
                            const string& preceding_text);

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
    return New<ContextualTranslation>(
        translation, input, preceding_text, grammar_.get());
  }

 private:
  template <class Strategy>
  an<Sentence> MakeSentenceWithStrategy(const WordGraph& graph,
                                        size_t total_length,
                                        const string& preceding_text);

  const Language* language_;
  the<Grammar> grammar_;
  Compare compare_;
};

}  // namespace rime

#endif  // RIME_POET_H_
