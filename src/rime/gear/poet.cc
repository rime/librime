//
// Copyright RIME Developers
// Distributed under the BSD License
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <functional>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/dict/vocabulary.h>
#include <rime/gear/grammar.h>
#include <rime/gear/poet.h>

namespace rime {

inline static Grammar* create_grammar(Config* config) {
  if (auto* grammar = Grammar::Require("grammar")) {
    return grammar->Create(config);
  }
  return nullptr;
}

Poet::Poet(const Language* language, Config* config, Compare compare)
    : language_(language),
      grammar_(create_grammar(config)),
      compare_(compare) {}

Poet::~Poet() {}

bool Poet::LeftAssociateCompare(const Sentence& one, const Sentence& other) {
  return one.weight() < other.weight() || (  // left associate if even
      one.weight() == other.weight() && (
          one.size() > other.size() || (  // less components is more favorable
              one.size() == other.size() &&
              std::lexicographical_compare(one.syllable_lengths().begin(),
                                           one.syllable_lengths().end(),
                                           other.syllable_lengths().begin(),
                                           other.syllable_lengths().end()))));
}

// keep the best sentence candidate per last phrase
using SentenceCandidates = hash_map<string, of<Sentence>>;

template <int N>
static vector<of<Sentence>> find_top_candidates(
    const SentenceCandidates& candidates, Poet::Compare compare) {
  vector<of<Sentence>> top;
  top.reserve(N + 1);
  for (const auto& candidate : candidates) {
    auto pos = std::upper_bound(
        top.begin(), top.end(), candidate.second,
        [&](const an<Sentence>& a, const an<Sentence>& b) {
          return !compare(*a, *b);  // desc
        });
    if (pos - top.begin() >= N) continue;
    top.insert(pos, candidate.second);
    if (top.size() > N) top.pop_back();
  }
  return top;
}

an<Sentence> find_best_sentence(const SentenceCandidates& candidates,
                                Poet::Compare compare) {
  an<Sentence> best = nullptr;
  for (const auto& candidate : candidates) {
    if (!best || compare(*best, *candidate.second)) {
      best = candidate.second;
    }
  }
  return best;
}

using UpdateSetenceCandidate = function<void (const an<Sentence>& candidate)>;

struct BeamSearch {
  using State = SentenceCandidates;

  static constexpr int kMaxSentenceCandidates = 7;

  static void Initiate(State& initial_state, const Language* language) {
    initial_state.emplace("", New<Sentence>(language));
  }

  static void ForEachCandidate(const State& state,
                               Poet::Compare compare,
                               UpdateSetenceCandidate update) {
    auto top_candidates =
        find_top_candidates<kMaxSentenceCandidates>(state, compare);
    for (const auto& candidate : top_candidates) {
      update(candidate);
    }
  }

  static an<Sentence>& BestSentenceToUpdate(State& state,
                                            const an<Sentence>& new_sentence) {
    const auto& key = new_sentence->components().back().text;
    return state[key];
  }

  static an<Sentence> BestSentence(const State& final_state,
                                   Poet::Compare compare) {
    return find_best_sentence(final_state, compare);
  }
};

struct DynamicProgramming {
  using State = an<Sentence>;

  static void Initiate(State& initial_state, const Language* language) {
    initial_state = New<Sentence>(language);
  }

  static void ForEachCandidate(const State& state,
                               Poet::Compare compare,
                               UpdateSetenceCandidate update) {
    update(state);
  }

  static an<Sentence>& BestSentenceToUpdate(State& state,
                                            const an<Sentence>& new_sentence) {
    return state;
  }

  static an<Sentence> BestSentence(const State& final_state,
                                   Poet::Compare compare) {
    return final_state;
  }
};

template <class Strategy>
an<Sentence> Poet::MakeSentenceWithStrategy(const WordGraph& graph,
                                            size_t total_length,
                                            const string& preceding_text) {
  map<int, typename Strategy::State> sentences;
  Strategy::Initiate(sentences[0], language_);
  for (const auto& w : graph) {
    size_t start_pos = w.first;
    if (sentences.find(start_pos) == sentences.end())
      continue;
    DLOG(INFO) << "start pos: " << start_pos;
    for (const auto& x : w.second) {
      size_t end_pos = x.first;
      if (start_pos == 0 && end_pos == total_length)
        continue;  // exclude single words from the result
      DLOG(INFO) << "end pos: " << end_pos;
      bool is_rear = end_pos == total_length;
      // extend candidates with dict entries on a valid edge.
      auto& target(sentences[end_pos]);
      const auto& source(sentences[start_pos]);
      const DictEntryList& entries(x.second);
      auto extend_sentence_and_update =
          [&](const an<Sentence>& candidate) {
            for (const auto& entry : entries) {
              auto new_sentence = New<Sentence>(*candidate);
              new_sentence->Extend(
                  *entry, end_pos, is_rear, preceding_text, grammar_.get());
              auto& best_sentence =
                  Strategy::BestSentenceToUpdate(target, new_sentence);
              if (!best_sentence || compare_(*best_sentence, *new_sentence)) {
                DLOG(INFO) << "updated sentences " << end_pos << ") with "
                           << new_sentence->text() << " weight: "
                           << new_sentence->weight();
                best_sentence = std::move(new_sentence);
              }
            }
          };
    Strategy::ForEachCandidate(source, compare_, extend_sentence_and_update);
    }
  }
  auto found = sentences.find(total_length);
  if (found == sentences.end())
    return nullptr;
  else
    return Strategy::BestSentence(found->second, compare_);
}

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                size_t total_length,
                                const string& preceding_text) {
  return grammar_ ?
      MakeSentenceWithStrategy<BeamSearch>(
          graph, total_length, preceding_text) :
      MakeSentenceWithStrategy<DynamicProgramming>(
          graph, total_length, preceding_text);
}

}  // namespace rime
