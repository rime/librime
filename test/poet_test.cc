//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <rime/gear/poet.h>

#if !defined(_WIN32) || !defined(RIME_IMPORTS)
namespace rime {
namespace {

void AddWord(WordGraph& graph,
             int start,
             int end,
             const string& text,
             double weight) {
  auto entry = New<DictEntry>();
  entry->text = text;
  entry->weight = weight;
  graph[start][end].push_back(entry);
}

TEST(PoetTest, FullBeamKeepsEarlierTiesAndAcceptsBetterDuplicates) {
  WordGraph graph;
  AddWord(graph, 0, 1, "A", 0);
  // Two requested sentences give a beam of six lines.
  for (char suffix = 'a'; suffix <= 'g'; ++suffix)
    AddWord(graph, 1, 2, string(1, suffix), -1);
  AddWord(graph, 1, 2, "a", -2);
  AddWord(graph, 1, 2, "f", 0);

  Poet poet(nullptr, nullptr);
  const auto sentences = poet.MakeSentences(graph, 2, "", 2, 1.0);
  ASSERT_EQ(sentences.size(), 2);
  EXPECT_EQ(sentences[0]->text(), "Af");
  EXPECT_EQ(sentences[1]->text(), "Aa");
}

TEST(PoetTest, EqualWeightDuplicateKeepsEarlierSegmentation) {
  WordGraph graph;
  AddWord(graph, 0, 1, "a", 0);
  AddWord(graph, 0, 2, "ab", 0);
  AddWord(graph, 1, 3, "bc", 0);
  AddWord(graph, 2, 3, "c", 0);
  AddWord(graph, 2, 3, "d", -1);

  Poet poet(nullptr, nullptr);
  const auto sentences = poet.MakeSentences(graph, 3, "", 2, 1.0);
  ASSERT_EQ(sentences.size(), 2);
  EXPECT_EQ(sentences[0]->text(), "abc");
  EXPECT_EQ(sentences[0]->word_lengths(), (vector<size_t>{1, 2}));
  EXPECT_EQ(sentences[1]->text(), "abd");
}

}  // namespace
}  // namespace rime
#endif
