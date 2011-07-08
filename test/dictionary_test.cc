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
  //rime::DictEntryIterator it = dict.Lookup("zhong");
  //ASSERT_FALSE(it == rime::DictEntryIterator());
  //EXPECT_EQ("中", it->text());
  //EXPECT_EQ("zhong", it->raw_code_sequence());
  ASSERT_TRUE(dict.Unload());
}
