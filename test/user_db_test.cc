//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
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

TEST(RimeUserDbValueTest, Denormal) {
  const string denormal_entry = "c=16 d=9.88131e-324 t=1449225";
  UserDbValue v(denormal_entry);
  EXPECT_EQ(16, v.commits);
  EXPECT_EQ(1449225u, v.tick);
  EXPECT_GE(v.dee, 0.0);
  EXPECT_FALSE(std::isnan(v.dee));
}

TEST(RimeUserDbTest, RecoverFromCorruptedDb) {
  UserDb::Component* component = UserDb::Require("userdb");
  ASSERT_TRUE(component);
  the<Db> db(component->Create("user_db_test"));
  ASSERT_TRUE(db);
  // Remove() cannot delete the lost/ directory RepairDB may have left
  // behind, so remove the whole directory for a deterministic fixture.
  std::filesystem::remove_all(db->file_path());
  ASSERT_FALSE(db->Exists());
  db->Open();
  ASSERT_TRUE(db->loaded());
  EXPECT_TRUE(db->Update("abc", "ZYX"));
  EXPECT_TRUE(db->Update("zyx", "ABC"));
  EXPECT_TRUE(db->Close());

  // simulate corruption: CURRENT refers to a removed manifest
  {
    std::ofstream current((db->file_path() / "CURRENT").string(),
                          std::ios::binary | std::ios::trunc);
    current << "MANIFEST-99999999\n";
  }
  // the corrupted db is automatically repaired on open
  EXPECT_TRUE(db->Open());
  ASSERT_TRUE(db->loaded());
  string value;
  EXPECT_TRUE(db->Fetch("abc", &value));
  EXPECT_EQ("ZYX", value);
  value.clear();
  EXPECT_TRUE(db->Fetch("zyx", &value));
  EXPECT_EQ("ABC", value);
  db->Close();
  std::filesystem::remove_all(db->file_path());
}
