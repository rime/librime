// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/impl/dictionary.h>

TEST(RimeDictionaryTest, TheWholePackage) {
  rime::Dictionary dict("dictionary_test");
  ASSERT_TRUE(dict.Compile("luna_pinyin.dict.yaml"));
  ASSERT_TRUE(dict.Load());
  // TODO:
  ASSERT_TRUE(dict.Unload());
}
