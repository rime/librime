//
// Copyright RIME Developers
// Distributed under the BSD License
//
// T9 增量音节图缓存（SyllableGraphCache）与全量 BuildSyllableGraph
// 的等价性测试：九键逐位追加数字串，断言增量扩展产出的音节图
// （顶点/边/补全/indices）与整串全量重建完全一致。
//
#include <gtest/gtest.h>
#include <math.h>
#include <random>
#include <rime/algo/syllabifier.h>
#include <rime/dict/prism.h>

using namespace rime;

namespace {

const char* kSyllables[] = {"a",     "an",   "cha",  "chan", "chang",
                            "gan",   "han",  "hang", "na",   "tu",
                            "tuan"};

string T9Key(const string& syl) {
  string k;
  for (char c : syl) {
    if (c <= 'c')
      k += '2';
    else if (c <= 'f')
      k += '3';
    else if (c <= 'i')
      k += '4';
    else if (c <= 'l')
      k += '5';
    else if (c <= 'o')
      k += '6';
    else if (c <= 's')
      k += '7';
    else if (c <= 'v')
      k += '8';
    else
      k += '9';
  }
  return k;
}

class SyllabifierIncrementalTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    set<string> keyset;
    for (const char* s : kSyllables) {
      keyset.insert(T9Key(s));
    }
    prism_.reset(new Prism(path("syllabifier_incremental_test.bin")));
    prism_->Build(keyset);
  }

 protected:
  the<Prism> prism_;
};

bool SpellingPropsEqual(const EdgeProperties& a, const EdgeProperties& b) {
  return a.type == b.type && a.end_pos == b.end_pos &&
         fabs(a.credibility - b.credibility) < 1e-9 &&
         a.is_correction == b.is_correction &&
         a.tips == b.tips &&
         a.ambiguous_source_positions == b.ambiguous_source_positions;
}

void ExpectGraphsEqual(const SyllableGraph& inc, const SyllableGraph& full) {
  EXPECT_EQ(full.input_length, inc.input_length);
  EXPECT_EQ(full.interpreted_length, inc.interpreted_length);
  ASSERT_EQ(full.vertices.size(), inc.vertices.size());
  for (const auto& v : full.vertices) {
    auto it = inc.vertices.find(v.first);
    ASSERT_FALSE(it == inc.vertices.end())
        << "missing vertex " << v.first;
    EXPECT_EQ(v.second, it->second) << "vertex " << v.first;
  }
  ASSERT_EQ(full.edges.size(), inc.edges.size());
  for (const auto& sv : full.edges) {
    auto it = inc.edges.find(sv.first);
    ASSERT_FALSE(it == inc.edges.end()) << "missing edge start " << sv.first;
    ASSERT_EQ(sv.second.size(), it->second.size());
    for (const auto& ev : sv.second) {
      auto eit = it->second.find(ev.first);
      ASSERT_FALSE(eit == it->second.end())
          << "missing edge end " << ev.first << " at start " << sv.first;
      ASSERT_EQ(ev.second.size(), eit->second.size());
      for (const auto& sp : ev.second) {
        auto sit = eit->second.find(sp.first);
        ASSERT_FALSE(sit == eit->second.end())
            << "missing syllable " << sp.first << " on edge [" << sv.first
            << ", " << ev.first << ")";
        EXPECT_TRUE(SpellingPropsEqual(sit->second, sp.second))
            << "spelling props differ on edge [" << sv.first << ", "
            << ev.first << ")";
      }
    }
  }
  // indices 指向边属性，逐项比对结构与其指向的属性
  ASSERT_EQ(full.indices.size(), inc.indices.size());
  for (const auto& iv : full.indices) {
    auto it = inc.indices.find(iv.first);
    ASSERT_FALSE(it == inc.indices.end()) << "missing index at " << iv.first;
    ASSERT_EQ(iv.second.size(), it->second.size());
    for (const auto& sidx : iv.second) {
      auto sit = it->second.find(sidx.first);
      ASSERT_FALSE(sit == it->second.end());
      ASSERT_EQ(sidx.second.size(), sit->second.size());
      for (size_t i = 0; i < sidx.second.size(); ++i) {
        EXPECT_TRUE(SpellingPropsEqual(*sit->second[i], *sidx.second[i]));
      }
    }
  }
}

void RunIncrementalEquivalenceScenario(Prism& prism, const string& seq) {
  // 每轮新建 Syllabifier（对应真实流程中每次 Query 新建的
  // ScriptSyllabifier），缓存跨轮传入
  SyllableGraphCache cache;
  for (size_t len = 1; len <= seq.size(); ++len) {
    const string input = seq.substr(0, len);
    Syllabifier incremental(" '", true, false);
    Syllabifier reference(" '", true, false);
    SyllableGraph g_inc, g_full;
    int consumed_inc =
        incremental.BuildSyllableGraph(input, prism, &g_inc, &cache);
    int consumed_full = reference.BuildSyllableGraph(input, prism, &g_full);
    EXPECT_EQ(consumed_full, consumed_inc) << "input '" << input << "'";
    ExpectGraphsEqual(g_inc, g_full);
  }
}

TEST_F(SyllabifierIncrementalTest, IncrementalBuildMatchesFullBuild) {
  // 依次覆盖：完整解释、尾部补全边、多音节切分歧义等场景
  const char* sequences[] = {
      "24264",      // chang（完整解释）
      "242642",     // chang + 尾部不可解释（触发补全边）
      "42642",      // gan/han + 42
      "8826",       // tuan
      "24268826",   // chan + tuan
      "242",        // cha/cha? 前缀两可
      "62",         // na/na
      "242642642",  // 长序列（chan gan ...）
  };
  for (const char* seq : sequences) {
    RunIncrementalEquivalenceScenario(*prism_, seq);
  }
}

TEST_F(SyllabifierIncrementalTest, RandomSequencesMatchFullBuild) {
  std::mt19937 rng(20260905);
  std::uniform_int_distribution<int> digit('2', '9');
  for (int trial = 0; trial < 30; ++trial) {
    string seq;
    for (int i = 0, n = 6 + static_cast<int>(rng() % 12); i < n; ++i)
      seq.push_back(static_cast<char>(digit(rng)));
    RunIncrementalEquivalenceScenario(*prism_, seq);
  }
}

}  // namespace
