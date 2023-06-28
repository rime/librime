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

// internal data structure used during the sentence making process.
// the output line of the algorithm is transformed to an<Sentence>.
struct Line {
  // be sure the pointer to predecessor Line object is stable. it works since
  // pointer to values stored in std::map and std::unordered_map are stable.
  const Line* predecessor;
  // as long as the word graph lives, pointers to entries are valid.
  const DictEntry* entry;
  size_t end_pos;
  double weight;

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

const Line Line::kEmpty{nullptr, nullptr, 0, 0.0};

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

// keep the best line candidate per last phrase
using LineCandidates = hash_map<string, Line>;

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
an<Sentence> Poet::MakeSentenceWithStrategy(const WordGraph& graph,
                                            size_t total_length,
                                            const string& preceding_text) {
  map<int, typename Strategy::State> states;
  Strategy::Initiate(states[0]);
  for (const auto& sv : graph) {
    size_t start_pos = sv.first;
    if (states.find(start_pos) == states.end())
      continue;
    DLOG(INFO) << "start pos: " << start_pos;
    const auto& source_state = states[start_pos];
    const auto update = [this, &states, &sv, start_pos, total_length,
                         &preceding_text](const Line& candidate) {
      for (const auto& ev : sv.second) {
        size_t end_pos = ev.first;
        if (start_pos == 0 && end_pos == total_length)
          continue;  // exclude single word from the result
        DLOG(INFO) << "end pos: " << end_pos;
        bool is_rear = end_pos == total_length;
        auto& target_state = states[end_pos];
        // extend candidates with dict entries on a valid edge.
        const DictEntryList& entries = ev.second;
        for (const auto& entry : entries) {
          const string& context =
              candidate.empty() ? preceding_text : candidate.context();
          double weight = candidate.weight +
                          Grammar::Evaluate(context, entry->text, entry->weight,
                                            is_rear, grammar_.get());
          Line new_line{&candidate, entry.get(), end_pos, weight};
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
  auto found = states.find(total_length);
  if (found == states.end() || found->second.empty())
    return nullptr;
  const Line& best = Strategy::BestLineInState(found->second, compare_);
  auto sentence = New<Sentence>(language_);
  for (const auto* c : best.components()) {
    if (!c->entry)
      continue;
    sentence->Extend(*c->entry, c->end_pos, c->weight);
  }
  return sentence;
}

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                size_t total_length,
                                const string& preceding_text) {
  return grammar_ ? MakeSentenceWithStrategy<BeamSearch>(graph, total_length,
                                                         preceding_text)
                  : MakeSentenceWithStrategy<DynamicProgramming>(
                        graph, total_length, preceding_text);
}

}  // namespace rime
