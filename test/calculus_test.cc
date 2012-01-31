// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/algo/calculus.h>

const char* kTransliteration = "xlit abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char* kTransformation = "xform/^([zcs])h(.*)$/$1$2/";
const char* kErasion = "erase/^[czs]h[aoe]ng?$/";

TEST(RimeCalculusTest, Transliteration) {
  rime::Calculus calc;
  rime::scoped_ptr<rime::Calculation> c(calc.Parse(kTransliteration));
  ASSERT_TRUE(c);
  rime::Spelling si("abracadabra");
  rime::Spelling so;
  EXPECT_TRUE(c->Apply(si, &so));
  EXPECT_EQ("ABRACADABRA", so.str);
}

TEST(RimeCalculusTest, Transformation) {
  rime::Calculus calc;
  rime::scoped_ptr<rime::Calculation> c(calc.Parse(kTransformation));
  ASSERT_TRUE(c);
  rime::Spelling si("shang");
  rime::Spelling so;
  EXPECT_TRUE(c->Apply(si, &so));
  EXPECT_EQ("sang", so.str);
  // non-matching case
  si.str = "bang";
  so.str.clear();
  EXPECT_FALSE(c->Apply(si, &so));
}

TEST(RimeCalculusTest, Erasion) {
  rime::Calculus calc;
  rime::scoped_ptr<rime::Calculation> c(calc.Parse(kErasion));
  ASSERT_TRUE(c);
  EXPECT_FALSE(c->addition());
  EXPECT_TRUE(c->deletion());
  rime::Spelling si("shang");
  rime::Spelling so;
  EXPECT_TRUE(c->Apply(si, &so));
  // non-matching case
  si.str = "bang";
  so.str.clear();
  EXPECT_FALSE(c->Apply(si, &so));
}
