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

const Line Line::kEmpty{nullptr, nullptr, 0, 0.0, 0};

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

bool Poet::CompareWeight(const Line& one, const Line& other) {
  return one.weight < other.weight;
}

// returns true if one is less than other.
bool Poet::LeftAssociateCompare(const Line& one, const Line& other) {
  if (one.weight < other.weight)
    return true;
  if (one.weight == other.weight) {
    auto one_word_lens = one.word_lengths();
    auto other_word_lens = other.word_lengths();
    // less words is more favorable
    if (one_word_lens.size() > other_word_lens.size())
      return true;
    if (one_word_lens.size() == other_word_lens.size()) {
      return std::lexicographical_compare(
          one_word_lens.begin(), one_word_lens.end(), other_word_lens.begin(),
          other_word_lens.end());
    }
  }
  return false;
}

template <int N>
static vector<const Line*> find_top_candidates(const LineCandidates& candidates,
                                               Poet::Compare compare) {
  vector<const Line*> top;
  top.reserve(N + 1);
  for (const auto& candidate : candidates) {
    auto pos = std::upper_bound(
        top.begin(), top.end(), &candidate.second,
        [&](const Line* a, const Line* b) { return compare(*b, *a); });  // desc
    if (pos - top.begin() >= N)
      continue;
    top.insert(pos, &candidate.second);
    if (top.size() > N)
      top.pop_back();
  }
  return top;
}

using UpdateLineCandidate = function<void(const Line& candidate)>;

struct BeamSearch {
  using State = LineCandidates;

  static constexpr int kMaxLineCandidates = 7;

  static void Initiate(State& initial_state) {
    initial_state.emplace("", Line::kEmpty);
  }

  static void ForEachCandidate(const State& state,
                               Poet::Compare compare,
                               UpdateLineCandidate update) {
    auto top_candidates =
        find_top_candidates<kMaxLineCandidates>(state, compare);
    for (const auto* candidate : top_candidates) {
      update(*candidate);
    }
  }

  static Line& BestLineToUpdate(State& state, const Line& new_line) {
    const auto& key = new_line.last_word();
    return state[key];
  }

  static const Line& BestLineInState(const State& final_state,
                                     Poet::Compare compare) {
    const Line* best = nullptr;
    for (const auto& candidate : final_state) {
      if (!best || compare(*best, candidate.second)) {
        best = &candidate.second;
      }
    }
    return best ? *best : Line::kEmpty;
  }
};

struct DynamicProgramming {
  using State = Line;

  static void Initiate(State& initial_state) { initial_state = Line::kEmpty; }

  static void ForEachCandidate(const State& state,
                               Poet::Compare compare,
                               UpdateLineCandidate update) {
    update(state);
  }

  static Line& BestLineToUpdate(State& state, const Line& new_line) {
    return state;
  }

  static const Line& BestLineInState(const State& final_state,
                                     Poet::Compare compare) {
    return final_state;
  }
};

template <class Strategy>
an<Sentence> Poet::MakeSentenceWithStrategy(
    const WordGraph& graph,
    size_t total_length,
    const string& preceding_text,
    size_t covered_len,
    map<int, typename Strategy::State>* states) {
  map<int, typename Strategy::State> fresh_states;
  if (!states)
    states = &fresh_states;
  if (states->empty())
    Strategy::Initiate((*states)[0]);
  for (const auto& sv : graph) {
    size_t start_pos = sv.first;
    if (states->find(start_pos) == states->end())
      continue;
    DLOG(INFO) << "start pos: " << start_pos;
    const auto& source_state = (*states)[start_pos];
    const auto update = [this, states, &sv, start_pos, total_length,
                         covered_len, &preceding_text](const Line& candidate) {
      for (const auto& ev : sv.second) {
        size_t end_pos = ev.first;
        if (end_pos <= covered_len)
          continue;  // already decoded in a previous round
        DLOG(INFO) << "end pos: " << end_pos;
        bool is_rear = end_pos == total_length;
        auto& target_state = (*states)[end_pos];
        // extend candidates with dict entries on a valid edge.
        const DictEntryList& entries = ev.second;
        for (const auto& entry : entries) {
          const string& context =
              candidate.empty() ? preceding_text : candidate.context();
          double weight = candidate.weight +
                          Grammar::Evaluate(context, entry->text, entry->weight,
                                            is_rear, grammar_.get());
          Line new_line{&candidate, entry, end_pos, weight};
          Line& best = Strategy::BestLineToUpdate(target_state, new_line);
          if (best.empty() || compare_(best, new_line)) {
            DLOG(INFO) << "updated line ending at " << end_pos
                       << " with text: ..." << new_line.last_word()
                       << " weight: " << new_line.weight;
            best = new_line;
          }
        }
      }
    };
    Strategy::ForEachCandidate(source_state, compare_, update);
  }
  auto found = states->find(total_length);
  if (found == states->end() || found->second.empty())
    return nullptr;
  // the single-word covering of the whole input is handled by the word
  // candidates, not the sentence; skipping it when building the states would
  // tie the states to the input length, which breaks the incremental decode
  // that reuses them across key presses
  const Line* best = nullptr;
  Strategy::ForEachCandidate(
      found->second, compare_, [&](const Line& candidate) {
        bool single_word = candidate.end_pos == total_length &&
                           candidate.components().lines.size() == 1;
        if (single_word)
          return;
        if (!best || compare_(*best, candidate)) {
          best = &candidate;
        }
      });
  if (!best)
    return nullptr;
  auto sentence = New<Sentence>(language_);
  for (const auto* c : best->components()) {
    if (!c->entry)
      continue;
    sentence->Extend(*c->entry, c->end_pos, c->weight);
  }
  return sentence;
}

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                size_t total_length,
                                const string& preceding_text) {
  return MakeSentence(graph, total_length, preceding_text, 0, nullptr);
}

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                size_t total_length,
                                const string& preceding_text,
                                size_t covered_len,
                                IncrementalStates* states) {
  // without caller-provided states there is nothing to extend, and a
  // non-zero covered_len would skip edges and silently drop candidates
  if (!states) {
    covered_len = 0;
  }
  if (grammar_) {
    return MakeSentenceWithStrategy<BeamSearch>(
        graph, total_length, preceding_text, covered_len,
        states ? &states->beam : nullptr);
  }
  return MakeSentenceWithStrategy<DynamicProgramming>(
      graph, total_length, preceding_text, covered_len,
      states ? &states->dp : nullptr);
}

// Make `max_sentences` sentences using beam search and dp on word graph.
//
// There is no strategy because it unconditionally use grammar.
deque<an<Sentence>> Poet::MakeSentences(const WordGraph& graph,
                                        size_t total_length,
                                        const string& preceding_text,
                                        size_t max_sentences,
                                        double cutoff_threshold) {
  size_t beam_width =
      max_sentences * 3;  // allow more possibilities during search
  using State = std::list<Line>;
  map<int, State> states;
  states[0].push_back(Line::kEmpty);
  for (const auto& sv : graph) {
    size_t start_pos = sv.first;
    if (states.find(start_pos) == states.end())
      continue;

    const auto& source_state = states[start_pos];
    for (const auto& ev : sv.second) {
      size_t end_pos = ev.first;
      if (start_pos == 0 && end_pos == total_length)
        continue;
      const DictEntryList& entries = ev.second;
      bool is_rear = end_pos == total_length;
      auto& target_state = states[end_pos];

      for (const auto& source_line : source_state) {
        for (const auto& entry : entries) {
          const string& context =
              source_line.empty() ? preceding_text : source_line.context();
          double weight = source_line.weight +
                          Grammar::Evaluate(context, entry->text, entry->weight,
                                            is_rear, grammar_.get());
          size_t new_hash = source_line.text_hash;
          for (char c : entry->text) {
            new_hash = new_hash * 31 + c;
          }
          Line new_line{&source_line, entry, end_pos, weight, new_hash};

          // dedup by text hash
          auto dup = std::find_if(
              target_state.begin(), target_state.end(),
              [&](const Line& l) { return l.text_hash == new_line.text_hash; });
          if (dup != target_state.end()) {
            if (new_line.weight > dup->weight) {
              target_state.erase(dup);
            } else {
              continue;
            }
          }

          // insert in descending order of weight
          auto it = std::find_if(
              target_state.begin(), target_state.end(),
              [&](const Line& l) { return l.weight < new_line.weight; });
          target_state.insert(it, new_line);
          if (target_state.size() > beam_width)
            target_state.pop_back();
        }
      }
    }
  }

  auto found = states.find(total_length);
  if (found == states.end() || found->second.empty())
    return {};

  deque<an<Sentence>> results;
  double last_weight;
  double acceleration = 1.0 - 1.0 / (double)max_sentences;
  auto iter = found->second.begin();
  for (size_t i = 0; iter != found->second.end() && i < max_sentences;
       ++i, ++iter) {
    const auto& candidate = *iter;
    double cur_weight = candidate.weight;
    if (i > 0) {
      // idea: if the current sentence is, on average, not too rare when
      // compared to last sentence, we should consider it too
      if (fabs(cur_weight - last_weight) / fabs(last_weight) >
          cutoff_threshold) {
        break;
      }
      // but don't deviate too far from the first weight by accelerating
      // the cutoff threshold. cutoff_threshold becomes
      // ~0.36*cutoff_threshold after N candidates are added.
      cutoff_threshold *= acceleration;
    }
    last_weight = cur_weight;
    auto sentence = New<Sentence>(language_);
    for (const auto* c : candidate.components()) {
      if (!c->entry)
        continue;
      sentence->Extend(*c->entry, c->end_pos, c->weight);
    }
    results.emplace_back(sentence);
  }
  return results;
}

}  // namespace rime
