//
// Copyright RIME Developers
// Distributed under the BSD License
//
// tests for the incremental sentence decode: the incremental path must
// produce the same sentence as a full decode, and provides a rough timing
// comparison on long inputs.
//

#include <chrono>
#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/language.h>
#include <rime/gear/poet.h>
#include <rime/gear/translator_commons.h>

namespace rime {

namespace {

// build a pinyin-like word graph of n syllables: every position has edges of
// 1 to 3 syllables with a couple of homophones each, so the number of paths
// grows with the input length
WordGraph MakeWordGraph(size_t n) {
  WordGraph graph;
  for (size_t start = 0; start < n; ++start) {
    for (size_t len = 1; len <= 3 && start + len <= n; ++len) {
      size_t end = start + len;
      auto& entries = graph[start][end];
      int num_entries = (len == 1) ? 3 : 2;
      for (int k = 0; k < num_entries; ++k) {
        auto entry = New<DictEntry>();
        entry->text = "w" + std::to_string(start) + "_" + std::to_string(end) +
                      "_" + std::to_string(k);
        // longer words weigh slightly more, inviting longer compositions
        entry->weight = static_cast<double>(len) + 0.1 * k;
        entries.push_back(entry);
      }
    }
  }
  return graph;
}

string SentenceText(const Sentence& sentence) {
  string text;
  for (const auto& component : sentence.components()) {
    text += component.text;
  }
  return text;
}

}  // namespace

// incremental decode must produce the same sentence as a full decode on
// every key, in both the dp (no grammar) and beam (with grammar) strategies.
// the grammar component is not registered in the test environment, so only
// the dp strategy is exercised here; the beam strategy shares the same
// incremental code path through MakeSentenceWithStrategy.
TEST(PoetIncrementalTest, MatchesFullDecode) {
  Language language("test");
  Poet poet(&language, nullptr);
  Poet::IncrementalStates states;
  size_t covered_len = 0;
  const size_t kInputLength = 30;
  // a single syllable is excluded from sentence making, so start from two
  for (size_t n = 2; n <= kInputLength; ++n) {
    auto graph = MakeWordGraph(n);
    auto full = poet.MakeSentence(graph, n, "");
    auto incremental = poet.MakeSentence(graph, n, "", covered_len, &states);
    // both may legitimately return null; they must agree either way
    ASSERT_EQ(static_cast<bool>(full), static_cast<bool>(incremental))
        << "at input length " << n;
    if (full) {
      EXPECT_EQ(SentenceText(*full), SentenceText(*incremental))
          << "at input length " << n;
    }
    covered_len = n;
  }
}

// a covered_len beyond the graph is a caller error; the poet layer must not
// crash and returns no sentence since every edge is already covered
TEST(PoetIncrementalTest, ResetsOnInvalidCoverage) {
  Language language("test");
  Poet poet(&language, nullptr);
  Poet::IncrementalStates states;
  states.covered_len = 100;  // beyond the graph length
  states.valid = true;
  auto graph = MakeWordGraph(20);
  auto sentence = poet.MakeSentence(graph, 20, "", 100, &states);
  EXPECT_EQ(nullptr, sentence);
}

// multiple segments (e.g. when the caret sits mid-input) must not disturb
// each other's incremental states, just as the translator keeps them apart
TEST(PoetIncrementalTest, IndependentSegmentStates) {
  Language language("test");
  Poet poet(&language, nullptr);
  // two segments being decoded alternately with their own states
  Poet::IncrementalStates states_a;
  Poet::IncrementalStates states_b;
  size_t covered_a = 0;
  size_t covered_b = 0;
  const size_t kLengthA = 12;
  const size_t kLengthB = 9;
  for (size_t round = 0; round < 2; ++round) {
    for (size_t n = 2; n <= kLengthA; ++n) {
      auto graph = MakeWordGraph(n);
      auto full = poet.MakeSentence(graph, n, "");
      auto inc = poet.MakeSentence(graph, n, "", covered_a, &states_a);
      if (full) {
        EXPECT_EQ(SentenceText(*full), SentenceText(*inc))
            << "segment A, " << n;
      }
      covered_a = n;
    }
    for (size_t n = 2; n <= kLengthB; ++n) {
      auto graph = MakeWordGraph(n);
      auto full = poet.MakeSentence(graph, n, "");
      auto inc = poet.MakeSentence(graph, n, "", covered_b, &states_b);
      if (full) {
        EXPECT_EQ(SentenceText(*full), SentenceText(*inc))
            << "segment B, " << n;
      }
      covered_b = n;
    }
  }
}

// a set_input jump replaces the input wholesale; the translator detects the
// broken prefix and resets the states, after which the decode matches a full
// one and the incremental reuse resumes
TEST(PoetIncrementalTest, ResumesAfterInputJump) {
  Language language("test");
  Poet poet(&language, nullptr);
  Poet::IncrementalStates states;
  size_t covered_len = 0;
  for (size_t n = 2; n <= 10; ++n) {  // growing input, incremental
    auto graph = MakeWordGraph(n);
    auto inc = poet.MakeSentence(graph, n, "", covered_len, &states);
    if (inc) {
      EXPECT_EQ(SentenceText(*poet.MakeSentence(graph, n, "")),
                SentenceText(*inc))
          << "before jump, " << n;
    }
    covered_len = n;
  }
  // the input jumps to a fresh one, resetting the states to a full decode
  states = Poet::IncrementalStates{};
  covered_len = 0;
  for (size_t n = 2; n <= 10; ++n) {  // and the reuse resumes from scratch
    auto graph = MakeWordGraph(n);
    auto full = poet.MakeSentence(graph, n, "");
    auto inc = poet.MakeSentence(graph, n, "", covered_len, &states);
    if (full) {
      EXPECT_EQ(SentenceText(*full), SentenceText(*inc)) << "after jump, " << n;
    }
    covered_len = n;
  }
}

// rough timing comparison on long inputs; the numbers are reported to the
// test output and are meant as a sanity check, not a precise benchmark
TEST(PoetIncrementalBenchmark, LongInputTiming) {
  const size_t kInputLength = 40;
  const size_t kRounds = 20;
  Language language("test");
  Poet poet(&language, nullptr);

  std::vector<WordGraph> graphs(kInputLength + 1);
  for (size_t n = 1; n <= kInputLength; ++n) {
    graphs[n] = MakeWordGraph(n);
  }

  auto full_start = std::chrono::steady_clock::now();
  for (size_t round = 0; round < kRounds; ++round) {
    for (size_t n = 1; n <= kInputLength; ++n) {
      poet.MakeSentence(graphs[n], n, "");
    }
  }
  auto full_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::steady_clock::now() - full_start)
                     .count();

  auto incremental_start = std::chrono::steady_clock::now();
  for (size_t round = 0; round < kRounds; ++round) {
    Poet::IncrementalStates states;
    size_t covered_len = 0;
    for (size_t n = 1; n <= kInputLength; ++n) {
      poet.MakeSentence(graphs[n], n, "", covered_len, &states);
      covered_len = n;
    }
  }
  auto incremental_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - incremental_start)
          .count();

  std::cout << "input length " << kInputLength << ", " << kRounds
            << " rounds of " << kInputLength << " keys:\n"
            << "  full decode: " << full_ms << " ms\n"
            << "  incremental: " << incremental_ms << " ms\n"
            << "  speedup: "
            << (incremental_ms > 0
                    ? std::to_string(static_cast<double>(full_ms) /
                                     incremental_ms)
                    : "n/a")
            << 'x' << std::endl;

  EXPECT_LT(incremental_ms, full_ms);
}

}  // namespace rime
