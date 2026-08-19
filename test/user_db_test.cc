//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <rime/algo/syllabifier.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_db.h>

using namespace rime;

using TestDb = UserDbWrapper<TextDb>;

TEST(RimeUserDbTest, AccessRecordByKey) {
  TestDb db(path{"user_db_test.txt"}, "user_db_test");
  if (db.Exists())
    db.Remove();
  ASSERT_FALSE(db.Exists());
  db.Open();
  EXPECT_TRUE(db.loaded());
  EXPECT_TRUE(db.Update("abc", "ZYX"));
  EXPECT_TRUE(db.Update("zyx", "CBA"));
  EXPECT_TRUE(db.Update("zyx", "ABC"));
  string value;
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
  EXPECT_TRUE(db.Close());
  ASSERT_FALSE(db.loaded());
}

TEST(RimeUserDbTest, Query) {
  TestDb db(path{"user_db_test.txt"}, "user_db_test");
  if (db.Exists())
    db.Remove();
  ASSERT_FALSE(db.Exists());
  db.Open();
  EXPECT_TRUE(db.Update("abc", "ZYX"));
  EXPECT_TRUE(db.Update("abc\tdef", "ZYX WVU"));
  EXPECT_TRUE(db.Update("zyx", "ABC"));
  EXPECT_TRUE(db.Update("wvu", "DEF"));
  {
    an<DbAccessor> accessor = db.Query("abc");
    ASSERT_TRUE(bool(accessor));
    EXPECT_FALSE(accessor->exhausted());
    string key, value;
    EXPECT_TRUE(accessor->GetNextRecord(&key, &value));
    EXPECT_EQ("abc", key);
    EXPECT_EQ("ZYX", value);
    key.clear();
    value.clear();
    EXPECT_TRUE(accessor->GetNextRecord(&key, &value));
    EXPECT_EQ("abc\tdef", key);
    EXPECT_EQ("ZYX WVU", value);
    key.clear();
    value.clear();
    EXPECT_FALSE(accessor->GetNextRecord(&key, &value));
    // key, value contain invalid contents
    EXPECT_EQ("", key);
    EXPECT_EQ("", value);
  }
  {
    an<DbAccessor> accessor = db.Query("wvu\tt");
    ASSERT_TRUE(bool(accessor));
    EXPECT_TRUE(accessor->exhausted());
    string key, value;
    EXPECT_FALSE(accessor->GetNextRecord(&key, &value));
  }
  {
    an<DbAccessor> accessor = db.Query("z");
    ASSERT_TRUE(bool(accessor));
    EXPECT_FALSE(accessor->exhausted());
    string key, value;
    EXPECT_TRUE(accessor->GetNextRecord(&key, &value));
    EXPECT_EQ("zyx", key);
    EXPECT_EQ("ABC", value);
    EXPECT_FALSE(accessor->GetNextRecord(&key, &value));
  }
  db.Close();
}

TEST(RimeUserDbValueTest, PackUnpackRoundtripNormal) {
  UserDbValue original;
  original.commits = 42;
  original.dee = 1.23456;
  original.tick = 1000;

  string packed = original.Pack();
  EXPECT_EQ("c=42 d=1.23456 t=1000", packed);

  UserDbValue restored(packed);
  EXPECT_EQ(42, restored.commits);
  EXPECT_DOUBLE_EQ(1.23456, restored.dee);
  EXPECT_EQ(1000u, restored.tick);
}

TEST(RimeUserDbValueTest, PackUnpackRoundtripZero) {
  UserDbValue original;
  original.commits = 0;
  original.dee = 0.0;
  original.tick = 0;

  string packed = original.Pack();
  UserDbValue restored(packed);
  EXPECT_EQ(0, restored.commits);
  EXPECT_DOUBLE_EQ(0.0, restored.dee);
  EXPECT_EQ(0u, restored.tick);
}

TEST(RimeUserDbValueTest, PackUnpackRoundtripSmallNormal) {
  // A small but normal (non-denormal) positive value, just above min().
  UserDbValue original;
  original.commits = 7;
  original.dee = 3e-300;
  original.tick = 999;

  string packed = original.Pack();
  UserDbValue restored(packed);
  EXPECT_EQ(7, restored.commits);
  EXPECT_DOUBLE_EQ(3e-300, restored.dee);
  EXPECT_EQ(999u, restored.tick);
}

TEST(RimeUserDbValueTest, UnpackSurvivesDenormal) {
  // Simulate an entry written by a buggy older version that serialized a
  // denormal float. Unpack MUST NOT fail; it should treat the denormal as
  // effectively zero (below the representable normal range).
  const string denormal_entry = "c=16 d=9.88131e-324 t=1449225";
  UserDbValue v(denormal_entry);
  EXPECT_EQ(16, v.commits);
  EXPECT_EQ(1449225u, v.tick);
  // dee should not throw or leave the field uninitialized; it must be
  // representable as a normal non-negative double (0 is acceptable).
  EXPECT_GE(v.dee, 0.0);
  EXPECT_FALSE(std::isnan(v.dee));
}

TEST(RimeUserDbValueTest, PackThenUnpackDenormalIsClamped) {
  // Setting dee to a denormal in memory and Pack()ing it must produce a
  // string that Unpack() can parse back without error — the round-trip must
  // not lose the record.
  UserDbValue original;
  original.commits = 5;
  original.dee = std::numeric_limits<double>::denorm_min();  // ~4.94e-324
  original.tick = 12345;

  string packed = original.Pack();
  UserDbValue restored;
  ASSERT_TRUE(restored.Unpack(packed))
      << "Unpack failed on packed string: " << packed;
  EXPECT_EQ(5, restored.commits);
  EXPECT_EQ(12345u, restored.tick);
  EXPECT_GE(restored.dee, 0.0);
  EXPECT_FALSE(std::isnan(restored.dee));
}

TEST(RimeUserDbValueTest, UnpackRejectsGarbage) {
  // Fields that are not numbers at all should still be rejected (the
  // Unpack() contract is not "accept anything").
  UserDbValue v;
  EXPECT_FALSE(v.Unpack("c=abc d=xyz t=qqq"));
}
