//
// Copyright RIME Developers
// Distributed under the BSD License
//
// T9 增量造句（MakeSentenceIncremental）与全量 MakeSentence 的等价性测试。
//
// 原理：九键逐位追加输入时，WordGraph 的新桶逐轮追加进同一 graph 对象。
// 增量路径复用上一轮的前向搜索状态，仅重算旧终点并展开新桶。
// 本测试用确定性随机的 WordGraph 模拟逐键追加，断言两种路径产出的
// 整句候选（分词、权重、文本）完全一致。
//
#include <gtest/gtest.h>
#include <math.h>
#include <map>
#include <random>
#include <utility>
#include <vector>
#include <rime/dict/vocabulary.h>
#include <rime/gear/grammar.h>
#include <rime/gear/poet.h>
#include <rime/language.h>
#include <rime/registry.h>

using namespace rime;

namespace {

// 确定性的假 grammar，用于覆盖 BeamSearch 分支（有 grammar 时启用）
class FakeGrammar : public Grammar {
 public:
  double Query(const string& context,
               const string& word,
               bool is_rear) override {
    double h = 0.0;
    for (char c : context)
      h = h * 31.0 + c;
    for (char c : word)
      h = h * 131.0 + c;
    h = fmod(fabs(h), 1.0);
    // 句尾（is_rear）加分、句中减分，保证 is_rear 参与权重且可区分
    return -h - (is_rear ? 0.0 : 0.5);
  }
};

class FakeGrammarComponent : public Grammar::Component {
 public:
  Grammar* Create(Config* config) override {
    return new FakeGrammar;  // 测试组件忽略 config
  }
};

// 模拟九键逐位追加：每轮对两个 graph 追加相同的桶，
// 增量路径（ginc + lattice）与全量参照（gfull）产出必须一致。
static void RunEquivalenceScenario(unsigned seed, size_t max_len) {
  Language lang("t9_test");
  Poet poet(&lang, nullptr);

  WordGraph ginc, gfull;
  an<Poet::Lattice> lattice;
  // 预先固化桶内容序列：增量图与全量图使用相同序列
  std::vector<std::map<std::pair<int, int>, DictEntryList>> buckets(
      max_len + 1);
  for (size_t len = 2; len <= max_len; ++len) {
    std::mt19937 gen(seed + len);
    for (size_t start = 0; start + 1 <= len - 1 && start < len; ++start) {
      std::uniform_int_distribution<int> count(1, 3);
      DictEntryList entries;
      for (int i = 0, n = count(gen); i < n; ++i) {
        auto entry = New<DictEntry>();
        entry->text = "w" + std::to_string(start) + "_" +
                      std::to_string(len) + "_" + std::to_string(i);
        std::uniform_real_distribution<double> weight(-8.0, -1.0);
        entry->weight = weight(gen);
        entries.push_back(entry);
      }
      buckets[len][{static_cast<int>(start), static_cast<int>(len)}] =
          std::move(entries);
    }
  }

  for (size_t len = 2; len <= max_len; ++len) {
    // 追加本轮新桶
    for (const auto& kv : buckets[len]) {
      DictEntryList& inc_bucket = ginc[kv.first.first][kv.first.second];
      ASSERT_TRUE(inc_bucket.empty());
      for (const auto& e : kv.second)
        inc_bucket.push_back(e);
      DictEntryList& full_bucket = gfull[kv.first.first][kv.first.second];
      ASSERT_TRUE(full_bucket.empty());
      for (const auto& e : kv.second)
        full_bucket.push_back(e);
    }

    // 增量路径（失败时回退全量，与 ScriptTranslation 的编排一致）
    an<Sentence> inc_sentence;
    if (lattice) {
      inc_sentence = poet.MakeSentenceIncremental(ginc, len, "", lattice);
    }
    if (!inc_sentence) {
      inc_sentence = poet.MakeSentence(ginc, len, "", &lattice);
    }
    // 全量参照
    an<Poet::Lattice> reference_lattice;
    an<Sentence> full_sentence =
        poet.MakeSentence(gfull, len, "", &reference_lattice);

    if (!full_sentence) {
      EXPECT_FALSE(inc_sentence) << "seed=" << seed << " len=" << len;
      continue;
    }
    ASSERT_TRUE(inc_sentence) << "seed=" << seed << " len=" << len;
    EXPECT_EQ(inc_sentence->text(), full_sentence->text())
        << "seed=" << seed << " len=" << len;
    EXPECT_EQ(inc_sentence->size(), full_sentence->size())
        << "seed=" << seed << " len=" << len;
    EXPECT_EQ(inc_sentence->word_lengths(), full_sentence->word_lengths())
        << "seed=" << seed << " len=" << len;
    EXPECT_NEAR(inc_sentence->weight(), full_sentence->weight(), 1e-12)
        << "seed=" << seed << " len=" << len;
  }
}

TEST(PoetIncrementalTest, DynamicProgrammingIncrementalMatchesFullRun) {
  // 确保无 grammar 组件：Poet 走 DynamicProgramming 分支
  Registry::instance().Unregister("grammar");
  for (unsigned seed = 1; seed <= 50; ++seed) {
    RunEquivalenceScenario(seed, 14);
  }
}

TEST(PoetIncrementalTest, BeamSearchIncrementalMatchesFullRun) {
  Registry::instance().Register("grammar", new FakeGrammarComponent);
  for (unsigned seed = 1; seed <= 50; ++seed) {
    RunEquivalenceScenario(seed, 14);
  }
}

TEST(PoetIncrementalTest, LongInputIncrementalMatchesFullRun) {
  Registry::instance().Unregister("grammar");
  for (unsigned seed = 100; seed <= 105; ++seed) {
    RunEquivalenceScenario(seed, 26);
  }
}

}  // namespace
