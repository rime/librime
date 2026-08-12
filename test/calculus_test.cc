//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <cmath>
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/algo/calculus.h>

// Combo Pinyin 3.0 key order
static const char* kComboPinyinKeyOrder = "SCZHLFGDBKTPIUÜANREO";

using namespace rime;

TEST(RimeCalculusTest, Transliteration) {
  Calculus calc;
  the<Calculation> c(
      calc.Parse("xlit abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  ASSERT_TRUE(bool(c));
  Spelling s("abracadabra");
  EXPECT_TRUE(c->Apply(&s));
  EXPECT_EQ("ABRACADABRA", s.str);
}

TEST(RimeCalculusTest, Transformation) {
  Calculus calc;
  the<Calculation> c(calc.Parse("xform/^([zcs])h(.*)$/$1$2/"));
  ASSERT_TRUE(bool(c));
  Spelling s("shang");
  EXPECT_TRUE(c->Apply(&s));
  EXPECT_EQ("sang", s.str);
  // non-matching case
  s.str = "bang";
  EXPECT_FALSE(c->Apply(&s));
}

TEST(RimeCalculusTest, Erasion) {
  Calculus calc;
  the<Calculation> c(calc.Parse("erase/^[czs]h[aoe]ng?$/"));
  ASSERT_TRUE(bool(c));
  EXPECT_FALSE(c->addition());
  EXPECT_TRUE(c->deletion());
  Spelling s("shang");
  EXPECT_TRUE(c->Apply(&s));
  EXPECT_EQ("", s.str);
  // non-matching case
  s.str = "bang";
  EXPECT_FALSE(c->Apply(&s));
}

TEST(RimeCalculusTest, Derivation) {
  Calculus calc;
  the<Calculation> c(calc.Parse("derive/^([zcs])h/$1/"));
  ASSERT_TRUE(bool(c));
  EXPECT_TRUE(c->addition());
  EXPECT_FALSE(c->deletion());
  Spelling s("shang");
  EXPECT_TRUE(c->Apply(&s));
  EXPECT_EQ("sang", s.str);
  // non-matching case
  s.str = "bang";
  EXPECT_FALSE(c->Apply(&s));
}

TEST(RimeCalculusTest, Abbreviation) {
  Calculus calc;
  the<Calculation> c(calc.Parse("abbrev/^([zcs]h).*$/$1/"));
  ASSERT_TRUE(bool(c));
  EXPECT_TRUE(c->addition());
  EXPECT_FALSE(c->deletion());
  Spelling s("shang");
  EXPECT_TRUE(c->Apply(&s));
  EXPECT_EQ("sh", s.str);
  EXPECT_EQ(rime::kAbbreviation, s.properties.type);
  EXPECT_DOUBLE_EQ(log(0.5), s.properties.credibility);
}

// 基礎雙鍵與多鍵並擊亂序重排 (Inversion & Interleaving)
TEST(RimeCalculusTest, ReorderBasicInversion) {
  Calculus calc;
  std::string formula = std::string("reorder ") + kComboPinyinKeyOrder;
  std::unique_ptr<Calculation> reorder(calc.Parse(formula));
  ASSERT_NE(reorder, nullptr);

  // 1. 聲母同手指倒錯：FZ (zh) -> ZF
  Spelling s1("FZ");
  EXPECT_TRUE(reorder->Apply(&s1));
  EXPECT_EQ(s1.str, "ZF");

  // 2. 音節內多鍵亂序：FZURO (zhong) -> ZFURO
  Spelling s2("FZURO");
  EXPECT_TRUE(reorder->Apply(&s2));
  EXPECT_EQ(s2.str, "ZFURO");

  // 3. 雙手並發極速交錯（右手先於左手）：UZRF (zhui) -> ZFUR
  Spelling s3("UZRF");
  EXPECT_TRUE(reorder->Apply(&s3));
  EXPECT_EQ(s3.str, "ZFUR");
}

// 已經符合標準鍵序的字符串（無須修改，應返回 false）
TEST(RimeCalculusTest, ReorderAlreadyOrdered) {
  Calculus calc;
  std::string formula = std::string("reorder ") + kComboPinyinKeyOrder;
  std::unique_ptr<Calculation> reorder(calc.Parse(formula));
  ASSERT_NE(reorder, nullptr);

  // 標準和弦碼：不應修改，Apply 返回 false
  Spelling s1("ZFURO");
  EXPECT_FALSE(reorder->Apply(&s1));
  EXPECT_EQ(s1.str, "ZFURO");

  Spelling s2("ZF");
  EXPECT_FALSE(reorder->Apply(&s2));
  EXPECT_EQ(s2.str, "ZF");
}

// 包含音節分隔符 '\'' 的連擊字符串 (Delimited Key Groups)
TEST(RimeCalculusTest, ReorderWithDelimiter) {
  Calculus calc;
  std::string formula = std::string("reorder ") + kComboPinyinKeyOrder;
  std::unique_ptr<Calculation> reorder(calc.Parse(formula));
  ASSERT_NE(reorder, nullptr);

  // 1. 第一組亂序，第二組已有序：FZURO'UN -> ZFURO'UN
  Spelling s1("FZURO'UN");
  EXPECT_TRUE(reorder->Apply(&s1));
  EXPECT_EQ(s1.str, "ZFURO'UN");

  // 2. 兩組均亂序：FZ'NU -> ZF'UN
  Spelling s2("FZ'NU");
  EXPECT_TRUE(reorder->Apply(&s2));
  EXPECT_EQ(s2.str, "ZF'UN");

  // 3. 分隔符位於開頭與末尾的邊界情況
  Spelling s3("'FZ");
  EXPECT_TRUE(reorder->Apply(&s3));
  EXPECT_EQ(s3.str, "'ZF");

  Spelling s4("FZ'");
  EXPECT_TRUE(reorder->Apply(&s4));
  EXPECT_EQ(s4.str, "ZF'");
}

// 去重模式 (dedup) 驗證
TEST(RimeCalculusTest, ReorderDeduplication) {
  Calculus calc;
  std::string formula_no_dedup = std::string("reorder ") + kComboPinyinKeyOrder;
  std::string formula_dedup =
      std::string("reorder ") + kComboPinyinKeyOrder + " dedup";

  std::unique_ptr<Calculation> reorder_no_dedup(calc.Parse(formula_no_dedup));
  std::unique_ptr<Calculation> reorder_dedup(calc.Parse(formula_dedup));

  ASSERT_NE(reorder_no_dedup, nullptr);
  ASSERT_NE(reorder_dedup, nullptr);

  // 無 dedup 模式：重複按鍵保留（若是順序的則返回 false）
  Spelling s1("ZZFFUU");
  EXPECT_FALSE(reorder_no_dedup->Apply(&s1));
  EXPECT_EQ(s1.str, "ZZFFUU");

  // 有 dedup 模式：重複按鍵被壓縮去重，且返回 true（觸發了修改）
  Spelling s2("ZZFFUU");
  EXPECT_TRUE(reorder_dedup->Apply(&s2));
  EXPECT_EQ(s2.str, "ZFU");

  // 有 dedup 模式 + 亂序：FFZZUU -> ZFU
  Spelling s3("FFZZUU");
  EXPECT_TRUE(reorder_dedup->Apply(&s3));
  EXPECT_EQ(s3.str, "ZFU");
}

// 非鍵序內字符（如標點、大寫字母）的隔離保護
TEST(RimeCalculusTest, ReorderNonOrderCharacters) {
  Calculus calc;
  std::unique_ptr<Calculation> reorder(calc.Parse("reorder zyx"));
  ASSERT_NE(reorder, nullptr);

  // 1. 字符 'a' 不在 "zyx" 中，作為分隔符：xzy - a - xzy -> zyx - a - zyx
  Spelling s1("xzyaxzy");
  EXPECT_TRUE(reorder->Apply(&s1));
  EXPECT_EQ(s1.str, "zyxazyx");

  // 2. 大寫字母 A, B 不在 "zyx" 中，原樣保留：
  // xzy - A - xzy - B -> zyx - A - zyx - B
  Spelling s2("xzyAxzyB");
  EXPECT_TRUE(reorder->Apply(&s2));
  EXPECT_EQ(s2.str, "zyxAzyxB");
}

// 非鍵序內字符（如標點、大寫字母）的隔離保護，開啓去重模式
TEST(RimeCalculusTest, ReorderNonOrderCharactersWithDedup) {
  Calculus calc;
  // 加上 dedup 參數
  std::unique_ptr<Calculation> reorder(calc.Parse("reorder zyx dedup"));
  ASSERT_NE(reorder, nullptr);

  // 'a' 爲分隔符，xzxy 去重重排 -> zyx，xzy 重排 -> zyx
  Spelling s1("xzxyaxzy");
  EXPECT_TRUE(reorder->Apply(&s1));
  EXPECT_EQ(s1.str, "zyxazyx");
}

// Calculus 工廠解析器 (Parse) 的參數校驗
TEST(RimeCalculusTest, ReorderParseValidation) {
  Calculus calc;

  // 缺少參數 -> 返回 nullptr
  EXPECT_EQ(calc.Parse("reorder"), nullptr);

  // order 字符串爲空 -> 返回 nullptr
  EXPECT_EQ(calc.Parse("reorder "), nullptr);

  // 合法參數
  EXPECT_NE(calc.Parse("reorder zyx"), nullptr);
  EXPECT_NE(calc.Parse("reorder zyx dedup"), nullptr);
}
