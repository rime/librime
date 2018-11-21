//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/21.
//
#include <algorithm>
#include <utility>
#include <gtest/gtest.h>
#include <rime/dict/prism.h>
#include <rime/algo/syllabifier.h>
#include <rime/algo/corrector.h>
#include <iostream>

class RimeCorrectorTest : public ::testing::Test {
 public:
  void SetUp() override {
    rime::vector<rime::string> syllables;
    syllables.emplace_back("a");      // 0 == id
    syllables.emplace_back("an");     // 1
    syllables.emplace_back("cha");    // 2
    syllables.emplace_back("chan");   // 3
    syllables.emplace_back("chang");  // 4
    syllables.emplace_back("gan");    // 5
    syllables.emplace_back("han");    // 6
    syllables.emplace_back("hang");   // 7
    syllables.emplace_back("na");     // 8
    syllables.emplace_back("tu");     // 9
    syllables.emplace_back("tuan");   // 10
    std::sort(syllables.begin(), syllables.end());
    for (size_t i = 0; i < syllables.size(); ++i) {
      syllable_id_[syllables[i]] = i;
    }

    prism_ = std::make_unique<rime::Prism>("corrector_test.prism.bin");
    corrector_ = std::make_unique<rime::Corrector>("corrector_test.corrector.bin");
    rime::set<rime::string> keyset;
    std::copy(syllables.begin(), syllables.end(),
              std::inserter(keyset, keyset.begin()));
    prism_->Build(keyset);
    corrector_->Build(keyset, nullptr, 0, 0);
  }

  virtual void TearDown() {
  }

 protected:
  rime::map<rime::string, rime::SyllableId> syllable_id_;
  rime::the<rime::Prism> prism_;
  rime::the<rime::Corrector> corrector_;
};

TEST_F(RimeCorrectorTest, CaseChun) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("chun");
  s.BuildSyllableGraph(input, *prism_, &g, corrector_.get());
  std::cout << g.input_length << std::endl;
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(1));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[1]);
  rime::SpellingMap& sp(g.edges[0][1]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["chan"]));
  EXPECT_EQ(rime::kNormalSpelling, sp[0].type);
  EXPECT_EQ(1.0, sp[0].credibility);
}
