//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <utility>
#include <gtest/gtest.h>
#include <rime/algo/algebra.h>
#include <rime/dict/prism.h>
#include <rime/algo/syllabifier.h>

class RimeSyllabifierTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    rime::vector<rime::string> syllables;
    syllables.push_back("a");      // 0 == id
    syllables.push_back("an");     // 1
    syllables.push_back("cha");    // 2
    syllables.push_back("chan");   // 3
    syllables.push_back("chang");  // 4
    syllables.push_back("gan");    // 5
    syllables.push_back("han");    // 6
    syllables.push_back("hang");   // 7
    syllables.push_back("na");     // 8
    syllables.push_back("tu");     // 9
    syllables.push_back("tuan");   // 10
    std::sort(syllables.begin(), syllables.end());
    for (size_t i = 0; i < syllables.size(); ++i) {
      syllable_id_[syllables[i]] = i;
    }

    rime::path file_path("syllabifier_test.bin");
    prism_.reset(new rime::Prism(file_path));
    rime::set<rime::string> keyset;
    std::copy(syllables.begin(), syllables.end(),
              std::inserter(keyset, keyset.begin()));
    prism_->Build(keyset);
  }

  virtual void TearDown() {
    std::error_code ec;
    std::filesystem::remove("syllabifier_test.bin", ec);
  }

 protected:
  rime::map<rime::string, rime::SyllableId> syllable_id_;
  rime::the<rime::Prism> prism_;
};

TEST_F(RimeSyllabifierTest, CaseAlpha) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("a");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(1));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[1]);
  rime::SpellingMap& sp(g.edges[0][1]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["a"]));
  EXPECT_EQ(rime::kNormalSpelling, sp[0].type);
  EXPECT_EQ(0.0, sp[0].credibility);
}

TEST_F(RimeSyllabifierTest, CaseFailure) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("ang");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length() - 1, g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_TRUE(g.vertices.end() == g.vertices.find(1));
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(2));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[2]);
  rime::SpellingMap& sp(g.edges[0][2]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["an"]));
}

TEST_F(RimeSyllabifierTest, CaseChangan) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("changan");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(4, g.vertices.size());
  // not c'han'gan or c'hang'an
  EXPECT_TRUE(g.vertices.end() == g.vertices.find(1));
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(4));
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(5));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[4]);
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[5]);
  // chan, chang but not cha
  rime::EndVertexMap& e0(g.edges[0]);
  EXPECT_EQ(2, e0.size());
  ASSERT_FALSE(e0.end() == e0.find(4));
  ASSERT_FALSE(e0.end() == e0.find(5));
  EXPECT_FALSE(e0[4].end() == e0[4].find(syllable_id_["chan"]));
  EXPECT_FALSE(e0[5].end() == e0[5].find(syllable_id_["chang"]));
  // gan$
  rime::EndVertexMap& e4(g.edges[4]);
  EXPECT_EQ(1, e4.size());
  ASSERT_FALSE(e4.end() == e4.find(7));
  EXPECT_FALSE(e4[7].end() == e4[7].find(syllable_id_["gan"]));
  // an$
  rime::EndVertexMap& e5(g.edges[5]);
  EXPECT_EQ(1, e5.size());
  ASSERT_FALSE(e5.end() == e5.find(7));
  EXPECT_FALSE(e5[7].end() == e5[7].find(syllable_id_["an"]));
}

TEST_F(RimeSyllabifierTest, CaseTuan) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("tuan");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(3, g.vertices.size());
  // both tu'an and tuan
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(2));
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(4));
  EXPECT_EQ(rime::kAmbiguousSpelling, g.vertices[2]);
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[4]);
  rime::EndVertexMap& e0(g.edges[0]);
  EXPECT_EQ(2, e0.size());
  ASSERT_FALSE(e0.end() == e0.find(2));
  ASSERT_FALSE(e0.end() == e0.find(4));
  EXPECT_FALSE(e0[2].end() == e0[2].find(syllable_id_["tu"]));
  EXPECT_FALSE(e0[4].end() == e0[4].find(syllable_id_["tuan"]));
  // an$
  rime::EndVertexMap& e2(g.edges[2]);
  EXPECT_EQ(1, e2.size());
  ASSERT_FALSE(e2.end() == e2.find(4));
  EXPECT_FALSE(e2[4].end() == e2[4].find(syllable_id_["an"]));
}

TEST_F(RimeSyllabifierTest, CaseChainingAmbiguity) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("anana");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(input.length() + 1, g.vertices.size());
}

TEST_F(RimeSyllabifierTest, TransposedSyllableGraph) {
  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("changan");
  s.BuildSyllableGraph(input, *prism_, &g);
  ASSERT_FALSE(g.indices.end() == g.indices.find(0));
  EXPECT_EQ(2, g.indices[0].size());
  EXPECT_FALSE(g.indices[0].end() == g.indices[0].find(syllable_id_["chan"]));
  EXPECT_FALSE(g.indices[0].end() == g.indices[0].find(syllable_id_["chang"]));
  ASSERT_EQ(1, g.indices[0][syllable_id_["chan"]].size());
  ASSERT_FALSE(NULL == g.indices[0][syllable_id_["chan"]][0]);
  EXPECT_EQ(4, g.indices[0][syllable_id_["chan"]][0]->end_pos);
}

TEST_F(RimeSyllabifierTest, TrimLeadingDelimiters) {
  rime::Syllabifier s(" '");
  rime::SyllableGraph g;
  const rime::string input("''a");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(3));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[1]);
  rime::SpellingMap& sp(g.edges[0][3]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["a"]));
  EXPECT_EQ(rime::kNormalSpelling, sp[0].type);
  EXPECT_EQ(0.0, sp[0].credibility);
}

TEST_F(RimeSyllabifierTest, TrimTrailingDelimiters) {
  rime::Syllabifier s(" '");
  rime::SyllableGraph g;
  const rime::string input("a''");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(3));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[1]);
  rime::SpellingMap& sp(g.edges[0][3]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["a"]));
  EXPECT_EQ(rime::kNormalSpelling, sp[0].type);
  EXPECT_EQ(0.0, sp[0].credibility);
}

TEST_F(RimeSyllabifierTest, TrimBothLeadingAndTrailingDelimiters) {
  rime::Syllabifier s(" '");
  rime::SyllableGraph g;
  const rime::string input("''a''");
  s.BuildSyllableGraph(input, *prism_, &g);
  EXPECT_EQ(input.length(), g.input_length);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(2, g.vertices.size());
  ASSERT_FALSE(g.vertices.end() == g.vertices.find(5));
  EXPECT_EQ(rime::kNormalSpelling, g.vertices[1]);
  rime::SpellingMap& sp(g.edges[0][5]);
  EXPECT_EQ(1, sp.size());
  ASSERT_FALSE(sp.end() == sp.find(syllable_id_["a"]));
  EXPECT_EQ(rime::kNormalSpelling, sp[0].type);
  EXPECT_EQ(0.0, sp[0].credibility);
}

// Tests that the spelling cache is used correctly when the same spelling_id
// is encountered from multiple BFS vertices (ambiguous parsing).
// Builds a local syllabary that includes "n" as a standalone syllable to
// exercise the deeper ambiguous path: a + n + a + na.
// "a" spelling_id is queried at vertices 0 (miss) and 2,4 (hit).
// "n" spelling_id is queried at vertices 1 (miss) and 3 (hit).
TEST_F(RimeSyllabifierTest, AmbiguousCacheHit) {
  rime::vector<rime::string> syllables;
  syllables.push_back("a");
  syllables.push_back("an");
  syllables.push_back("n");
  syllables.push_back("na");
  std::sort(syllables.begin(), syllables.end());

  rime::map<rime::string, rime::SyllableId> sid;
  for (size_t i = 0; i < syllables.size(); ++i) {
    sid[syllables[i]] = i;
  }

  rime::path file_path("syllabifier_cache_test.bin");
  auto test_prism = rime::New<rime::Prism>(file_path);
  test_prism->Remove();
  rime::set<rime::string> keyset;
  std::copy(syllables.begin(), syllables.end(),
            std::inserter(keyset, keyset.begin()));
  EXPECT_TRUE(test_prism->Build(keyset));

  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("anana");
  int result = s.BuildSyllableGraph(input, *test_prism, &g);
  EXPECT_EQ(input.length(), result);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_EQ(input.length() + 1, g.vertices.size());

  // Verify key vertices and edges for the a-n-a-na path
  EXPECT_FALSE(g.edges.end() == g.edges.find(0));
  EXPECT_FALSE(g.edges.end() == g.edges.find(1));
  EXPECT_FALSE(g.edges.end() == g.edges.find(2));
  EXPECT_FALSE(g.vertices.end() == g.vertices.find(1));
  EXPECT_FALSE(g.vertices.end() == g.vertices.find(3));

  // a@0→1
  auto& e0 = g.edges[0];
  EXPECT_FALSE(e0.end() == e0.find(1));
  EXPECT_FALSE(e0[1].end() == e0[1].find(sid["a"]));
  // n@1→2
  auto& e1 = g.edges[1];
  EXPECT_FALSE(e1.end() == e1.find(2));
  EXPECT_FALSE(e1[2].end() == e1[2].find(sid["n"]));
  // a@2→3
  auto& e2 = g.edges[2];
  EXPECT_FALSE(e2.end() == e2.find(3));
  EXPECT_FALSE(e2[3].end() == e2[3].find(sid["a"]));
  // na@3→5
  auto& e3 = g.edges[3];
  EXPECT_FALSE(e3.end() == e3.find(5));
  EXPECT_FALSE(e3[5].end() == e3[5].find(sid["na"]));

  test_prism->Remove();
}

// Test with spelling algebra (Script) to exercise the full cached
// QuerySpelling path with a non-null spelling_map.
TEST_F(RimeSyllabifierTest, SpellingAlgebraCache) {
  rime::set<rime::string> keyset;
  keyset.insert("zhi");
  keyset.insert("li");
  keyset.insert("shi");

  rime::Script script;
  {
    rime::Spelling s("zhi");
    s.properties.type = rime::kNormalSpelling;
    script["zhi"].push_back(s);
  }
  {
    rime::Spelling s("zhi");
    s.properties.type = rime::kAbbreviation;
    script["zh"].push_back(s);
  }
  {
    rime::Spelling s("li");
    s.properties.type = rime::kNormalSpelling;
    script["li"].push_back(s);
  }
  {
    rime::Spelling s("shi");
    s.properties.type = rime::kNormalSpelling;
    script["shi"].push_back(s);
  }

  rime::path file_path("syllabifier_algebra_test.bin");
  auto test_prism = rime::New<rime::Prism>(file_path);
  test_prism->Remove();
  EXPECT_TRUE(test_prism->Build(keyset, &script));
  EXPECT_TRUE(test_prism->Save());
  EXPECT_TRUE(test_prism->Load());

  rime::Syllabifier s;
  rime::SyllableGraph g;
  const rime::string input("zhili");
  int result = s.BuildSyllableGraph(input, *test_prism, &g);
  EXPECT_EQ(input.length(), result);
  EXPECT_EQ(input.length(), g.interpreted_length);
  EXPECT_FALSE(g.vertices.end() == g.vertices.find(3));
  EXPECT_FALSE(g.vertices.end() == g.vertices.find(5));
  EXPECT_FALSE(g.edges.end() == g.edges.find(3));
  EXPECT_FALSE(g.edges[3].end() == g.edges[3].find(5));

  test_prism->Remove();
}
