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

static vector<of<Sentence>> top_candidates(const SentenceCandidates& candidates,
                                           size_t n,
                                           Poet::Compare& compare) {
  vector<of<Sentence>> top;
  top.reserve(n + 1);
  for (const auto& candidate : candidates) {
    auto pos = std::upper_bound(
        top.begin(), top.end(), candidate.second,
        [&](const an<Sentence>& a, const an<Sentence>& b) {
          return !compare(*a, *b);  // desc
        });
    if (pos - top.begin() >= n) continue;
    top.insert(pos, candidate.second);
    if (top.size() > n) top.pop_back();
  }
  return top;
}

an<Sentence> find_best_sentence(const SentenceCandidates& candidates,
                                Poet::Compare& compare) {
  an<Sentence> best = nullptr;
  for (const auto& candidate : candidates) {
    if (!best || compare(*best, *candidate.second)) {
      best = candidate.second;
    }
  }
  return best;
}

constexpr int kMaxSentenceCandidates = 7;

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                size_t total_length,
                                const string& preceding_text) {
  map<int, SentenceCandidates> sentences;
  sentences[0].emplace("", New<Sentence>(language_));
  for (const auto& w : graph) {
    size_t start_pos = w.first;
    if (sentences.find(start_pos) == sentences.end())
      continue;
    DLOG(INFO) << "start pos: " << start_pos;
    auto top = top_candidates(
        sentences[start_pos], kMaxSentenceCandidates, compare_);
    for (const auto& candidate : top) {
      for (const auto& x : w.second) {
        size_t end_pos = x.first;
        if (start_pos == 0 && end_pos == total_length)
          continue;  // exclude single words from the result
        DLOG(INFO) << "end pos: " << end_pos;
        bool is_rear = end_pos == total_length;
        auto& target(sentences[end_pos]);
        const DictEntryList& entries(x.second);
        for (const auto& entry : entries) {
          auto new_sentence = New<Sentence>(*candidate);
          new_sentence->Extend(
              *entry, end_pos, is_rear, preceding_text, grammar_.get());
          const auto& key = new_sentence->components().back().text;
          auto& best_sentence = target[key];
          if (!best_sentence || compare_(*best_sentence, *new_sentence)) {
            DLOG(INFO) << "updated sentences " << end_pos << ") with "
                       << new_sentence->text() << " weight: "
                       << new_sentence->weight();
            best_sentence = std::move(new_sentence);
          }
        }
      }
    }
  }
  if (sentences.find(total_length) == sentences.end())
    return nullptr;
  else
    return find_best_sentence(sentences[total_length], compare_);
}

}  // namespace rime
