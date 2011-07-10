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
    if (!built_) {
      built_ = dict_.Compile("luna_pinyin.dict.yaml");
    }
    if (built_) {
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
  static bool built_;
};

rime::Dictionary RimeDictionaryTest::dict_("dictionary_test");
bool RimeDictionaryTest::built_ = false;

TEST_F(RimeDictionaryTest, Ready) {
  EXPECT_TRUE(dict_.loaded());
}

TEST_F(RimeDictionaryTest, Lookup) {
  ASSERT_TRUE(dict_.loaded());
  rime::DictEntryIterator it = dict_.Lookup("zhong");
  ASSERT_TRUE(it);
  EXPECT_EQ("中", it->text);
  ASSERT_EQ(1, it->codes.size());
  EXPECT_EQ("zhong", it->codes.ToString());
}
