// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-19 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/algo/algebra.h>

static const char* kTransliteration = "xlit/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/";
static const char* kTransformation = "xform/^([zcs])h(.*)$/$1$2/";

static const int kNumOfInstructions = 5;
static const char* kInstructions[kNumOfInstructions] = {
  "xform/^(\\l+)\\d$/$1/",
  "erase/^[wxy].*$/",
  "derive/^([zcs])h\(.*)$/$1$2/",
  "abbrev/^(\\l).+$/$1/",
  "abbrev/^([zcs]h).+$/$1/",
};

TEST(RimeAlgebraTest, SpellingManipulation) {
  rime::ConfigListPtr c(new rime::ConfigList);
  c->Append(rime::ConfigItemPtr(new rime::ConfigValue(kTransliteration)));
  c->Append(rime::ConfigItemPtr(new rime::ConfigValue(kTransformation)));
  rime::Projection p;
  ASSERT_TRUE(p.Load(c));

  std::string str("Shang");
  EXPECT_TRUE(p.Apply(&str));
  EXPECT_EQ("sang", str);
}

TEST(RimeAlgebraTest, Projection) {
  rime::ConfigListPtr c(new rime::ConfigList);
  for (int i = 0; i < kNumOfInstructions; ++i) {
    c->Append(rime::ConfigItemPtr(new rime::ConfigValue(kInstructions[i])));
  }
  rime::Projection p;
  ASSERT_TRUE(p.Load(c));

  rime::Script s;
  s.AddSyllable("zhang1");
  s.AddSyllable("chang2");
  s.AddSyllable("shang3");
  s.AddSyllable("shang4");
  s.AddSyllable("zang1");
  s.AddSyllable("cang2");
  s.AddSyllable("bang3");
  s.AddSyllable("wang4");

  EXPECT_TRUE(p.Apply(&s));
  EXPECT_EQ(14, s.size());
  EXPECT_TRUE(s.find("zhang") != s.end());
  EXPECT_TRUE(s.find("chang") != s.end());
  EXPECT_TRUE(s.find("shang") != s.end());
  EXPECT_TRUE(s.find("zang") != s.end());
  EXPECT_TRUE(s.find("cang") != s.end());
  EXPECT_TRUE(s.find("sang") != s.end());
  EXPECT_TRUE(s.find("bang") != s.end());
  EXPECT_TRUE(s.find("zh") != s.end());
  EXPECT_TRUE(s.find("ch") != s.end());
  EXPECT_TRUE(s.find("sh") != s.end());
  EXPECT_TRUE(s.find("z") != s.end());
  EXPECT_TRUE(s.find("c") != s.end());
  EXPECT_TRUE(s.find("s") != s.end());
  EXPECT_TRUE(s.find("b") != s.end());
  EXPECT_FALSE(s.find("wang") != s.end());
  EXPECT_FALSE(s.find("wang4") != s.end());
  EXPECT_FALSE(s.find("zhang1") != s.end());
  EXPECT_FALSE(s.find("bang3") != s.end());
  EXPECT_EQ(2, s["z"].size());
  EXPECT_EQ(1, s["zh"].size());
  ASSERT_EQ(2, s["sh"].size());
  EXPECT_EQ(rime::kAbbreviation, s["sh"][0].properties.type);
  EXPECT_GT(0.5001, s["sh"][0].properties.credibility);
}
