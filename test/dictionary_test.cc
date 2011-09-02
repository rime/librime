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

class RimeDictionaryTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    EZLOGGERFUNCTRACKER;
    if (!dict_.Exists()) {
      dict_.Compile("luna_pinyin.dict.yaml");
    }
    if (dict_.Exists()) {
      dict_.Load();
    }
  }
  virtual void TearDown() {
    EZLOGGERFUNCTRACKER;
    if (dict_.loaded())
      dict_.Unload();
  }
 protected:
  static rime::Dictionary dict_;
};

rime::Dictionary RimeDictionaryTest::dict_("dictionary_test");

TEST_F(RimeDictionaryTest, Ready) {
  EXPECT_TRUE(dict_.loaded());
}

TEST_F(RimeDictionaryTest, LookupWords) {
  ASSERT_TRUE(dict_.loaded());
  rime::DictEntryIterator it = dict_.LookupWords("zhong", false);
  ASSERT_FALSE(it.exhausted());
  EXPECT_EQ("中", it.Peek()->text);
  ASSERT_EQ(1, it.Peek()->code.size());
  rime::dictionary::RawCode raw_code;
  ASSERT_TRUE(dict_.Decode(it.Peek()->code, &raw_code));
  EXPECT_EQ("zhong", raw_code.ToString());
}

TEST_F(RimeDictionaryTest, PredictiveLookup) {
  ASSERT_TRUE(dict_.loaded());
  rime::DictEntryIterator it = dict_.LookupWords("z", true);
  ASSERT_FALSE(it.exhausted());
  EXPECT_EQ("咋", it.Peek()->text);
  ASSERT_EQ(1, it.Peek()->code.size());
  rime::dictionary::RawCode raw_code;
  ASSERT_TRUE(dict_.Decode(it.Peek()->code, &raw_code));
  EXPECT_EQ("za", raw_code.ToString());
}
