//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <rime/deployer.h>
#include <rime/dict/level_db.h>
#include <rime/lever/user_dict_manager.h>

namespace fs = std::filesystem;
using namespace rime;

class RimeUserDictManagerTest : public testing::Test {
 protected:
  void SetUp() override {
    created_ = fs::create_directory(test_dir_);
    ASSERT_TRUE(created_);
    deployer_.user_data_dir = test_dir_;
  }

  void TearDown() override {
    if (created_)
      fs::remove_all(test_dir_);
  }

  const path test_dir_{"user_dict_manager_test"};
  bool created_ = false;
  Deployer deployer_;
};

TEST_F(RimeUserDictManagerTest, ExcludesOnlyReservedTemporaryDatabase) {
  auto* component = UserDb::Require("userdb");
  ASSERT_NE(nullptr, component);
  const string extension = component->extension();
  for (const string& name : UserDictList{"regular", ".hidden", ".temporary"}) {
    UserDbWrapper<LevelDb> db(test_dir_ / (name + extension), name);
    ASSERT_TRUE(db.Open());
    ASSERT_TRUE(db.Close());
  }
  // Restore() can leave an empty temporary database after an interruption.
  const path temp_db = test_dir_ / (".temp" + extension);
  ASSERT_TRUE(fs::create_directory(temp_db));
  ASSERT_TRUE(fs::create_directory(test_dir_ / "not_a_userdb"));

  UserDictManager manager(&deployer_);
  UserDictList user_dicts;
  manager.GetUserDictList(&user_dicts);

  std::sort(user_dicts.begin(), user_dicts.end());
  const UserDictList expected{".hidden", ".temporary", "regular"};
  EXPECT_EQ(expected, user_dicts);
  // Listing must not delete or modify the reserved database.
  EXPECT_TRUE(fs::is_directory(temp_db));
  EXPECT_TRUE(fs::is_empty(temp_db));
}
