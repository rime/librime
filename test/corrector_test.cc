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
#include <memory>

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
    corrector_.reset(new rime::Corrector("corrector_simple_test.corrector.bin"));
    rime::set<rime::string> keyset;
    std::copy(syllables.begin(), syllables.end(),
              std::inserter(keyset, keyset.begin()));
    prism_->Build(keyset);
    corrector_->Build(keyset, nullptr, 0, 0);

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

    prism_.reset(new rime::Prism("corrector_test.prism.bin"));
    corrector_.reset(new rime::Corrector("corrector_test.corrector.bin"));
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

TEST_F(RimeCorrectorSearchTest, CaseNearSubstitute) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("chsng");
  s.BuildSyllableGraph(input, *prism_, &g, corrector_.get());
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(5));
  EXPECT_EQ(rime::kCorrection, g.vertices[5]);
  rime::SpellingMap& sp(g.edges[0][5]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["chang"]));
  EXPECT_EQ(rime::kCorrection, sp[0].type);
}
TEST_F(RimeCorrectorSearchTest, CaseFarSubstitute) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("chpng");
  s.BuildSyllableGraph(input, *prism_, &g, corrector_.get());
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(0, g.interpreted_length);
  EXPECT_EQ(1, g.vertices.size());
  ASSERT_TRUE(g.vertices.end() == g.vertices.find(5));
}
TEST_F(RimeCorrectorSearchTest, CaseTranspose) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("cahng");
  s.BuildSyllableGraph(input, *prism_, &g, corrector_.get());
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(5));
  EXPECT_EQ(rime::kCorrection, g.vertices[5]);
  rime::SpellingMap& sp(g.edges[0][5]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["chang"]));
  EXPECT_EQ(rime::kCorrection, sp[0].type);
}

TEST_F(RimeCorrectorSearchTest, CaseCorrectionSyllabify) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("chabgtyan");
  s.BuildSyllableGraph(input, *prism_, &g, corrector_.get());
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(3, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(9));
  rime::SpellingMap& sp1(g.edges[0][5]);
  EXPECT_EQ(1, sp1.size());
  ASSERT_FALSE(sp1.end() == sp1.find(syllable_id_["chang"]));
  EXPECT_EQ(rime::kCorrection, sp1[0].type);
  rime::SpellingMap& sp2(g.edges[5][9]);
  EXPECT_EQ(1, sp2.size());
  ASSERT_FALSE(sp2.end() == sp2.find(syllable_id_["tuan"]));
  EXPECT_EQ(rime::kCorrection, sp2[1].type);
}

TEST_F(RimeCorrectorTest, CaseMultipleEdges) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("changtu"); // chang'tu; correction: hang'tu
  s.BuildSyllableGraph(input, *prism_, &g, corrector_.get());
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(3, g.vertices.size());
  rime::SpellingMap& sp1(g.edges[0][5]);
  EXPECT_EQ(2, sp1.size());
  ASSERT_FALSE(sp1.end() == sp1.find(syllable_id_["chang"]));
  ASSERT_FALSE(sp1.end() == sp1.find(syllable_id_["hang"]));
  rime::SpellingMap& sp2(g.edges[5][7]);
  EXPECT_EQ(1, sp2.size());
  ASSERT_FALSE(sp2.end() == sp2.find(syllable_id_["tu"]));
}
