//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <boost/format.hpp>
#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/config.h>

using namespace rime;

class RimeConfigCompilerTest : public ::testing::Test {
 protected:
  RimeConfigCompilerTest() = default;

  virtual void SetUp() {
    component_.reset(new ConfigComponent);
    config_.reset(component_->Create("config_compiler_test"));
  }

  virtual void TearDown() {
  }

  the<Config::Component> component_;
  the<Config> config_;
};

TEST_F(RimeConfigCompilerTest, IncludeLocalReference) {
  for (size_t i = 0; i < 4; ++i) {
    auto prefix = boost::str(boost::format("include_local_reference/@%u/") % i);
    EXPECT_TRUE(config_->IsNull(prefix + "__include"));
    string actual;
    EXPECT_TRUE(config_->GetString(prefix + "terrans/player", &actual));
    EXPECT_EQ("slayers_boxer", actual);
    EXPECT_TRUE(config_->GetString(prefix + "protoss/player", &actual));
    EXPECT_EQ("grrrr", actual);
    EXPECT_TRUE(config_->GetString(prefix + "zerg/player", &actual));
    EXPECT_EQ("yellow", actual);
  }
}

TEST_F(RimeConfigCompilerTest, IncludeExternalReference) {
  const string& prefix = "include_external_reference/";
  EXPECT_TRUE(config_->IsNull(prefix + "terrans/__include"));
  int mineral = 0;
  EXPECT_TRUE(config_->GetInt(prefix + "terrans/tank/cost/mineral", &mineral));
  EXPECT_EQ(150, mineral);
  int gas = 0;
  EXPECT_TRUE(config_->GetInt(prefix + "terrans/tank/cost/gas", &gas));
  EXPECT_EQ(100, gas);
  EXPECT_TRUE(config_->IsNull(prefix + "protoss"));
  EXPECT_TRUE(config_->IsNull(prefix + "zerg"));
}

TEST_F(RimeConfigCompilerTest, IncludeExternalFile) {
  const string& prefix = "include_external_file/";
  EXPECT_TRUE(config_->IsNull(prefix + "__include"));
  EXPECT_TRUE(config_->IsMap(prefix + "terrans"));
  EXPECT_TRUE(config_->IsMap(prefix + "protoss"));
  EXPECT_TRUE(config_->IsMap(prefix + "zerg"));
  int number = 0;
  EXPECT_TRUE(config_->GetInt(prefix + "terrans/supply/produced", &number));
  EXPECT_EQ(28, number);
  EXPECT_TRUE(config_->IsList(prefix + "protoss/air_force"));
  string name;
  EXPECT_TRUE(config_->GetString(prefix + "zerg/queen", &name));
  EXPECT_EQ("Kerrigan", name);
}

TEST_F(RimeConfigCompilerTest, PatchReference) {
  const string& prefix = "patch_reference/";
  EXPECT_TRUE(config_->IsNull(prefix + "__patch"));
  EXPECT_EQ(4, config_->GetListSize(prefix + "battlefields"));
  string map;
  EXPECT_TRUE(config_->GetString(prefix + "battlefields/@3", &map));
  EXPECT_EQ("match point", map);
}

TEST_F(RimeConfigCompilerTest, PatchLiteral) {
  const string& prefix = "patch_literal/";
  EXPECT_TRUE(config_->IsNull(prefix + "__patch"));
  EXPECT_EQ(6, config_->GetListSize(prefix + "zerg/ground_units"));
  string unit;
  EXPECT_TRUE(config_->GetString(prefix + "zerg/ground_units/@5", &unit));
  EXPECT_EQ("lurker", unit);
  // assert that we patched on a copy of the included node
  EXPECT_EQ(5, config_->GetListSize("starcraft/zerg/ground_units"));
}

TEST_F(RimeConfigCompilerTest, PatchList) {
  const string& prefix = "patch_list/";
  EXPECT_TRUE(config_->IsNull(prefix + "protoss/__patch"));
  EXPECT_EQ(8, config_->GetListSize(prefix + "protoss/ground_units"));
  string unit;
  EXPECT_TRUE(config_->GetString(prefix + "protoss/ground_units/@6", &unit));
  EXPECT_EQ("dark templar", unit);
  EXPECT_TRUE(config_->GetString(prefix + "protoss/ground_units/@7", &unit));
  EXPECT_EQ("dark archon", unit);
  // assert that we patched on a copy of the included node
  EXPECT_EQ(6, config_->GetListSize("starcraft/protoss/ground_units"));
}

TEST_F(RimeConfigCompilerTest, DependencyChaining) {
  const string& prefix = "dependency_chaining/";
  EXPECT_TRUE(config_->IsNull(prefix + "alpha/__include"));
  EXPECT_TRUE(config_->IsNull(prefix + "beta/__include"));
  EXPECT_TRUE(config_->IsNull(prefix + "delta/__include"));
  string value;
  EXPECT_TRUE(config_->GetString(prefix + "alpha", &value));
  EXPECT_EQ("success", value);
  EXPECT_TRUE(config_->GetString(prefix + "beta", &value));
  EXPECT_EQ("success", value);
  EXPECT_TRUE(config_->GetString(prefix + "delta", &value));
  EXPECT_EQ("success", value);
}

// Unit test for https://github.com/rime/librime/issues/141
TEST_F(RimeConfigCompilerTest, DependencyPriorities) {
  const string& prefix = "dependency_priorities/";
  EXPECT_TRUE(config_->IsNull(prefix + "terrans/__include"));
  EXPECT_TRUE(config_->IsNull(prefix + "terrans/__patch"));
  EXPECT_TRUE(config_->IsNull(prefix + "protoss/__include"));
  EXPECT_TRUE(config_->IsNull(prefix + "protoss/__patch"));
  string player;
  EXPECT_TRUE(config_->GetString(prefix + "terrans/player", &player));
  EXPECT_EQ("nada", player);
  EXPECT_TRUE(config_->GetString(prefix + "protoss/player", &player));
  EXPECT_EQ("bisu", player);
}

// TODO: test failure cases
