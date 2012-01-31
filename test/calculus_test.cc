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

TEST(RimedCalculusTest, Transliteration) {
  rime::Calculus calc;
  rime::scoped_ptr<rime::Calculation> c(calc.Parse(kTransliteration));
  ASSERT_TRUE(c);
  rime::Spelling si("abracadabra");
  rime::Spelling so;
  EXPECT_TRUE(c->Apply(si, &so));
  EXPECT_EQ("ABRACADABRA", so.str);
}

