// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <rime/impl/syllablizer.h>
#include <rime/impl/user_db.h>

TEST(RimeUserDbTest, AccessRecordByKey) {
  rime::UserDb db("user_db_test");
  if (db.Exists())
    db.Remove();
  ASSERT_FALSE(db.Exists());
  db.Open();
  EXPECT_TRUE(db.loaded());
  EXPECT_TRUE(db.Update("abc", "ZYX"));
  EXPECT_TRUE(db.Update("zyx", "CBA"));
  EXPECT_TRUE(db.Update("zyx", "ABC"));
  std::string value;
  EXPECT_TRUE(db.Fetch("abc", &value));
  EXPECT_EQ("ZYX", value);
  value.clear();
  EXPECT_TRUE(db.Fetch("zyx", &value));
  EXPECT_EQ("ABC", value);
  value.clear();
  EXPECT_FALSE(db.Fetch("wvu", &value));
  EXPECT_TRUE(value.empty());
  value.clear();
  EXPECT_TRUE(db.Erase("zyx"));
  EXPECT_FALSE(db.Fetch("zyx", &value));
  EXPECT_TRUE(value.empty());
  db.Close();
  EXPECT_FALSE(db.loaded());
}

TEST(RimeUserDbTest, Query) {
  rime::UserDb db("user_db_test");
  if (db.Exists())
    db.Remove();
  ASSERT_FALSE(db.Exists());
  db.Open();
  EXPECT_TRUE(db.Update("abc", "ZYX"));
  EXPECT_TRUE(db.Update("abcdef", "ZYXWVU"));
  EXPECT_TRUE(db.Update("zyx", "ABC"));
  EXPECT_TRUE(db.Update("wvu", "DEF"));
  {
    rime::UserDbAccessor accessor(db.Query("abc"));
    EXPECT_FALSE(accessor.exhausted());
    std::string key, value;
    EXPECT_TRUE(accessor.Yield(&key, &value));
    EXPECT_EQ("abc", key);
    EXPECT_EQ("ZYX", value);
    key.clear();
    value.clear();
    EXPECT_TRUE(accessor.Yield(&key, &value));
    EXPECT_EQ("abcdef", key);
    EXPECT_EQ("ZYXWVU", value);
    key.clear();
    value.clear();
    EXPECT_FALSE(accessor.Yield(&key, &value));
    // key, value contain invalid contents
  }
  {
    rime::UserDbAccessor accessor(db.Query("wvut"));
    EXPECT_TRUE(accessor.exhausted());
    std::string key, value;
    EXPECT_FALSE(accessor.Yield(&key, &value));
  }
  {
    rime::UserDbAccessor accessor(db.Query("z"));
    EXPECT_FALSE(accessor.exhausted());
    std::string key, value;
    EXPECT_TRUE(accessor.Yield(&key, &value));
    EXPECT_EQ("zyx", key);
    EXPECT_EQ("ABC", value);
    EXPECT_FALSE(accessor.Yield(&key, &value));
  }
  db.Close();
}
