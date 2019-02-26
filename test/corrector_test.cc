//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/21.
//
#include <algorithm>
#include <memory>
#include <gtest/gtest.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/corrector.h>
#include <rime/dict/prism.h>
#include <utility>

class RimeCorrectorSearchTest : public ::testing::Test {
 public:
  void SetUp() override {
    rime::vector<rime::string> syllables;
    syllables.emplace_back("chang");  // 0
    syllables.emplace_back("tuan");   // 1
    std::sort(syllables.begin(), syllables.end());
    for (size_t i = 0; i < syllables.size(); ++i) {
      syllable_id_[syllables[i]] = i;
    }

    prism_.reset(new rime::Prism("corrector_simple_test.prism.bin"));
    rime::set<rime::string> keyset;
    std::copy(syllables.begin(), syllables.end(),
              std::inserter(keyset, keyset.begin()));
    prism_->Build(keyset);

    corrector_.reset(new rime::NearSearchCorrector);
  }
  void TearDown() override {}
 protected:
  rime::map<rime::string, rime::SyllableId> syllable_id_;
  rime::the<rime::Prism> prism_;
  rime::the<rime::Corrector> corrector_;
};

class RimeCorrectorTest : public ::testing::Test {
 public:
  void SetUp() override {
    rime::vector<rime::string> syllables;
    syllables.emplace_back("j");      // 0 == id
    syllables.emplace_back("ji");     // 1
    syllables.emplace_back("jie");    // 2
    syllables.emplace_back("ju");     // 3
    syllables.emplace_back("jue");    // 4
    syllables.emplace_back("shen");   // 5
    std::sort(syllables.begin(), syllables.end());
    for (size_t i = 0; i < syllables.size(); ++i) {
      syllable_id_[syllables[i]] = i;
    }

    prism_.reset(new rime::Prism("corrector_test.prism.bin"));
    rime::set<rime::string> keyset;
    std::copy(syllables.begin(), syllables.end(),
              std::inserter(keyset, keyset.begin()));
    prism_->Build(keyset);

    corrector_.reset(new rime::NearSearchCorrector);
  }

  virtual void TearDown() {
  }

 protected:
  rime::map<rime::string, rime::SyllableId> syllable_id_;
  rime::the<rime::Prism> prism_;
  rime::the<rime::Corrector> corrector_;
};

TEST_F(RimeCorrectorSearchTest, CaseNearSubstitute) {
  rime::Syllabifier s;
  s.EnableCorrection(corrector_.get());
  rime::SyllableGraph g;
  const rime::string input("chsng");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(5));
  rime::SpellingMap& sp(g.edges[0][5]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["chang"]));
}

TEST_F(RimeCorrectorSearchTest, CaseFarSubstitute) {
  rime::Syllabifier s;
  s.EnableCorrection(corrector_.get());
  rime::SyllableGraph g;
  const rime::string input("chpng");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(0, g.interpreted_length);
  EXPECT_EQ(1, g.vertices.size());
  ASSERT_TRUE(g.vertices.end() == g.vertices.find(5));
}

TEST_F(RimeCorrectorSearchTest, DISABLED_CaseTranspose) {
  rime::Syllabifier s;
  s.EnableCorrection(corrector_.get());
  rime::SyllableGraph g;
  const rime::string input("cahng");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(5));
  rime::SpellingMap& sp(g.edges[0][5]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["chang"]));
}

TEST_F(RimeCorrectorSearchTest, CaseCorrectionSyllabify) {
  rime::Syllabifier s;
  s.EnableCorrection(corrector_.get());
  rime::SyllableGraph g;
  const rime::string input("chabgtyan");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(3, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(9));
  rime::SpellingMap& sp1(g.edges[0][5]);
  EXPECT_EQ(1, sp1.size());
  ASSERT_FALSE(sp1.end() == sp1.find(syllable_id_["chang"]));
  ASSERT_TRUE(sp1[0].is_correction);
  rime::SpellingMap& sp2(g.edges[5][9]);
  EXPECT_EQ(1, sp2.size());
  ASSERT_FALSE(sp2.end() == sp2.find(syllable_id_["tuan"]));
  ASSERT_TRUE(sp2[1].is_correction);
}

TEST_F(RimeCorrectorTest, CaseMultipleEdges1) {
  rime::Syllabifier s;
  s.EnableCorrection(corrector_.get());
  rime::SyllableGraph g;
  const rime::string input("jiejue"); // jie'jue jie'jie jue'jue jue'jie
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  rime::SpellingMap& sp1(g.edges[0][3]);
  EXPECT_EQ(2, sp1.size());
  ASSERT_FALSE(sp1.end() == sp1.find(syllable_id_["jie"]));
  ASSERT_TRUE(sp1[syllable_id_["jie"]].type == rime::kNormalSpelling);
  ASSERT_FALSE(sp1.end() == sp1.find(syllable_id_["jue"]));
  ASSERT_TRUE(sp1[syllable_id_["jue"]].is_correction);
  rime::SpellingMap& sp2(g.edges[3][6]);
  EXPECT_EQ(2, sp2.size());
  ASSERT_FALSE(sp2.end() == sp2.find(syllable_id_["jie"]));
  ASSERT_TRUE(sp2[syllable_id_["jie"]].is_correction);
  ASSERT_FALSE(sp2.end() == sp2.find(syllable_id_["jue"]));
  ASSERT_TRUE(sp2[syllable_id_["jue"]].type == rime::kNormalSpelling);
}
