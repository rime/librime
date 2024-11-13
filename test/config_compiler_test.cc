//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <sstream>
#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/config.h>
#include <rime/algo/strings.h>

using namespace rime;

class RimeConfigCompilerTestBase : public ::testing::Test {
 protected:
  RimeConfigCompilerTestBase() = default;

  virtual string test_config_id() const = 0;

  virtual void SetUp() {
    component_.reset(new ConfigComponent<ConfigBuilder>);
    config_.reset(component_->Create(test_config_id()));
  }

  the<Config::Component> component_;
  the<Config> config_;
};

class RimeConfigCompilerTest : public RimeConfigCompilerTestBase {
 protected:
  string test_config_id() const override { return "config_compiler_test"; }
};

TEST_F(RimeConfigCompilerTest, IncludeLocalReference) {
  for (size_t i = 0; i < 4; ++i) {
    std::ostringstream oss;
    oss << "include_local_reference/@" << i << "/";
    const auto& prefix(oss.str());
    EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__include")));
    string actual;
    EXPECT_TRUE(config_->GetString(
        rime::strings::concat(prefix, "terrans/player"), &actual));
    EXPECT_EQ("slayers_boxer", actual);
    EXPECT_TRUE(config_->GetString(
        rime::strings::concat(prefix, "protoss/player"), &actual));
    EXPECT_EQ("grrrr", actual);
    EXPECT_TRUE(config_->GetString(rime::strings::concat(prefix, "zerg/player"),
                                   &actual));
    EXPECT_EQ("yellow", actual);
  }
}

TEST_F(RimeConfigCompilerTest, IncludeExternalReference) {
  string_view prefix = "include_external_reference/";
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "terrans/__include")));
  int mineral = 0;
  EXPECT_TRUE(config_->GetInt(
      rime::strings::concat(prefix, "terrans/tank/cost/mineral"), &mineral));
  EXPECT_EQ(150, mineral);
  int gas = 0;
  EXPECT_TRUE(config_->GetInt(
      rime::strings::concat(prefix, "terrans/tank/cost/gas"), &gas));
  EXPECT_EQ(100, gas);
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "protoss")));
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "zerg")));
}

TEST_F(RimeConfigCompilerTest, IncludeExternalFile) {
  string_view prefix = "include_external_file/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__include")));
  EXPECT_TRUE(config_->IsMap(rime::strings::concat(prefix, "terrans")));
  EXPECT_TRUE(config_->IsMap(rime::strings::concat(prefix, "protoss")));
  EXPECT_TRUE(config_->IsMap(rime::strings::concat(prefix, "zerg")));
  int number = 0;
  EXPECT_TRUE(config_->GetInt(
      rime::strings::concat(prefix, "terrans/supply/produced"), &number));
  EXPECT_EQ(28, number);
  EXPECT_TRUE(
      config_->IsList(rime::strings::concat(prefix, "protoss/air_force")));
  string name;
  EXPECT_TRUE(
      config_->GetString(rime::strings::concat(prefix, "zerg/queen"), &name));
  EXPECT_EQ("Kerrigan", name);
}

TEST_F(RimeConfigCompilerTest, PatchReference) {
  string_view prefix = "patch_reference/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__patch")));
  EXPECT_EQ(
      4, config_->GetListSize(rime::strings::concat(prefix, "battlefields")));
  string map;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "battlefields/@3"), &map));
  EXPECT_EQ("match point", map);
}

TEST_F(RimeConfigCompilerTest, PatchLiteral) {
  string_view prefix = "patch_literal/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__patch")));
  EXPECT_EQ(6, config_->GetListSize(
                   rime::strings::concat(prefix, "zerg/ground_units")));
  string unit;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "zerg/ground_units/@5"), &unit));
  EXPECT_EQ("lurker", unit);
  // assert that we patched on a copy of the included node
  EXPECT_EQ(5, config_->GetListSize("/starcraft/zerg/ground_units"));
}

TEST_F(RimeConfigCompilerTest, PatchList) {
  string_view prefix = "patch_list/";
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "protoss/__patch")));
  EXPECT_EQ(8, config_->GetListSize(
                   rime::strings::concat(prefix, "protoss/ground_units")));
  string unit;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "protoss/ground_units/@6"), &unit));
  EXPECT_EQ("dark templar", unit);
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "protoss/ground_units/@7"), &unit));
  EXPECT_EQ("dark archon", unit);
  // assert that we patched on a copy of the included node
  EXPECT_EQ(6, config_->GetListSize("/starcraft/protoss/ground_units"));
}

class RimeConfigDependencyTest : public RimeConfigCompilerTestBase {
 protected:
  string test_config_id() const override { return "config_dependency_test"; }
};

TEST_F(RimeConfigDependencyTest, DependencyChaining) {
  string_view prefix = "dependency_chaining/";
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "alpha/__include")));
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "beta/__include")));
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "delta/__include")));
  string value;
  EXPECT_TRUE(
      config_->GetString(rime::strings::concat(prefix, "alpha"), &value));
  EXPECT_EQ("success", value);
  EXPECT_TRUE(
      config_->GetString(rime::strings::concat(prefix, "beta"), &value));
  EXPECT_EQ("success", value);
  EXPECT_TRUE(
      config_->GetString(rime::strings::concat(prefix, "delta"), &value));
  EXPECT_EQ("success", value);
}

// Unit test for https://github.com/rime/librime/issues/141
TEST_F(RimeConfigDependencyTest, DependencyPriorities) {
  string_view prefix = "dependency_priorities/";
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "terrans/__include")));
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "terrans/__patch")));
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "protoss/__include")));
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "protoss/__patch")));
  string player;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/player"), &player));
  EXPECT_EQ("nada", player);
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "protoss/player"), &player));
  EXPECT_EQ("bisu", player);
}

class RimeConfigOptionalReferenceTest : public RimeConfigCompilerTestBase {
 protected:
  string test_config_id() const override {
    return "config_optional_reference_test";
  }
};

TEST_F(RimeConfigOptionalReferenceTest, OptionalReference) {
  string_view prefix = "/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__include")));
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__patch")));
  bool untouched;
  EXPECT_TRUE(
      config_->GetBool(rime::strings::concat(prefix, "untouched"), &untouched));
  EXPECT_TRUE(untouched);
}

class RimeConfigMergeTest : public RimeConfigCompilerTestBase {
 protected:
  string test_config_id() const override { return "config_merge_test"; }
};

TEST_F(RimeConfigMergeTest, AppendWithInclude) {
  string_view prefix = "append_with_include/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "list/__include")));
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "list/__append")));

  EXPECT_TRUE(config_->IsList(rime::strings::concat(prefix, "list")));
  EXPECT_EQ(6 + 2, config_->GetListSize(rime::strings::concat(prefix, "list")));
  string unit;
  EXPECT_TRUE(
      config_->GetString(rime::strings::concat(prefix, "list/@6"), &unit));
  EXPECT_EQ("dark templar", unit);
  EXPECT_TRUE(
      config_->GetString(rime::strings::concat(prefix, "list/@7"), &unit));
  EXPECT_EQ("dark archon", unit);

  // verify that we append to a copy and the original list is untouched
  EXPECT_EQ(6, config_->GetListSize("/starcraft/protoss/ground_units"));
}

TEST_F(RimeConfigMergeTest, AppendWithPatch) {
  string_view prefix = "append_with_patch/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__include")));
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__patch")));

  string player;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/player"), &player));
  EXPECT_EQ("slayers_boxer, nada", player);

  EXPECT_EQ(2, config_->GetListSize(
                   rime::strings::concat(prefix, "terrans/air_units")));
  string unit;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/air_units/@0"), &unit));
  EXPECT_EQ("wraith", unit);
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/air_units/@1"), &unit));
  EXPECT_EQ("battlecruiser", unit);

  EXPECT_EQ(6 + 2, config_->GetListSize(
                       rime::strings::concat(prefix, "protoss/ground_units")));

  // verify that we append to a copy and the original list is untouched
  EXPECT_TRUE(config_->GetString("/starcraft/terrans/player", &player));
  EXPECT_EQ("slayers_boxer", player);
  EXPECT_TRUE(config_->IsNull("/starcraft/terrans/air_units"));
  EXPECT_EQ(6, config_->GetListSize("/starcraft/protoss/ground_units"));
}

TEST_F(RimeConfigMergeTest, MergeTree) {
  string_view prefix = "merge_tree/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__include")));

  string player;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/player"), &player));
  EXPECT_EQ("slayers_boxer", player);
  EXPECT_TRUE(
      config_->IsNull(rime::strings::concat(prefix, "terrans/air_units")));
  EXPECT_TRUE(
      config_->IsList(rime::strings::concat(prefix, "terrans/ground_units")));
  EXPECT_EQ(5 + 2, config_->GetListSize(
                       rime::strings::concat(prefix, "terrans/ground_units")));
  string unit;
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/ground_units/@5"), &unit));
  EXPECT_EQ("medic", unit);
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "terrans/ground_units/@6"), &unit));
  EXPECT_EQ("goliath", unit);

  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "protoss/player"), &player));
  EXPECT_EQ("grrrr", player);
  EXPECT_TRUE(config_->IsNull(
      rime::strings::concat(prefix, "protoss/ground_units/__append")));
  EXPECT_TRUE(
      config_->IsList(rime::strings::concat(prefix, "protoss/ground_units")));
  EXPECT_EQ(6 + 2, config_->GetListSize(
                       rime::strings::concat(prefix, "protoss/ground_units")));
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "protoss/ground_units/@6"), &unit));
  EXPECT_EQ("dark templar", unit);
  EXPECT_TRUE(config_->GetString(
      rime::strings::concat(prefix, "protoss/ground_units/@7"), &unit));
  EXPECT_EQ("dark archon", unit);

  EXPECT_TRUE(config_->GetString(rime::strings::concat(prefix, "zerg/player"),
                                 &player));
  EXPECT_EQ("yellow", player);
  EXPECT_TRUE(
      config_->IsList(rime::strings::concat(prefix, "zerg/ground_units")));
  EXPECT_EQ(0, config_->GetListSize(
                   rime::strings::concat(prefix, "zerg/ground_units")));

  // verify that we merge to a copy and the original list is untouched
  EXPECT_TRUE(config_->IsNull("/starcraft/terrans/ground_units"));
  EXPECT_EQ(6, config_->GetListSize("/starcraft/protoss/ground_units"));
  EXPECT_EQ(5, config_->GetListSize("/starcraft/zerg/ground_units"));
}

class RimeConfigCircularDependencyTest : public RimeConfigCompilerTestBase {
 protected:
  string test_config_id() const override {
    return "config_circular_dependency_test";
  }
};

TEST_F(RimeConfigMergeTest, CreateListWithInplacePatch) {
  string_view prefix = "create_list_with_inplace_patch/";
  EXPECT_TRUE(
      config_->IsList(rime::strings::concat(prefix, "all_ground_units")));
  EXPECT_EQ(16, config_->GetListSize(
                    rime::strings::concat(prefix, "all_ground_units")));
}

TEST_F(RimeConfigCircularDependencyTest, BestEffortResolution) {
  string_view prefix = "test/";
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "__patch")));
  EXPECT_TRUE(config_->IsNull(rime::strings::concat(prefix, "work/__include")));
  string home;
  EXPECT_TRUE(config_->GetString(rime::strings::concat(prefix, "home"), &home));
  EXPECT_EQ("naive", home);
  string work;
  EXPECT_TRUE(config_->GetString(rime::strings::concat(prefix, "work"), &work));
  EXPECT_EQ("excited", work);
}
