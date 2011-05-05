// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-06 Zou xu <zouivex@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/config.h>

using namespace rime;

class RimeConfigTest : public ::testing::Test {
 protected:
  RimeConfigTest() : component_(NULL), config_(NULL) {}

  virtual void SetUp() {
    component_ = new ConfigComponent(".");
    config_ = component_->Create("test.yaml");
  }

  virtual void TearDown() {
    if (config_)
      delete config_;
    if (component_)
      delete component_;
  }

  Config::Component *component_;
  Config *config_;
};

TEST(RimeConfigComponentTest, RealCreationWorkflow) {
  // registration
  Component::Register("test_config", new ConfigComponent("."));
  // finding component
  Config::Component *cc = Config::Find("test_config");
  ASSERT_TRUE(cc);
  // creation
  scoped_ptr<Config> config(cc->Create("test.yaml"));
  EXPECT_TRUE(config);
  Component::Unregister("test_config");
}

TEST(RimeConfigItemTest, NullItem) {
  ConfigItem item;
  EXPECT_EQ(ConfigItem::kNull, item.type());
}

/*

// TODO: dynamically creating ConfigItem...

TEST(RimeConfigItemTest, BooleanSchalar) {
  ConfigValue config_value(true);
  EXPECT_EQ(ConfigItem::kScalar, config_value.type());
  bool value = false;
  EXPECT_TRUE(config_value.get<bool>(&value));
  EXPECT_EQ(true, value);
}

TEST(RimeConfigItemTest, IntSchalar) {
  ConfigValue config_value(123);
  EXPECT_EQ(ConfigItem::kScalar, config_value.type());
  int value = 0;
  EXPECT_TRUE(config_value.get<int>(&value));
  EXPECT_EQ(123, value);
}

TEST(RimeConfigItemTest, DoubleSchalar) {
  ConfigValue config_value(3.1415926);
  EXPECT_EQ(ConfigItem::kScalar, config_value.type());
  double value = 0;
  EXPECT_TRUE(config_value.get<double>(&value));
  EXPECT_EQ(3.1415926, value);
}

TEST(RimeConfigItemTest, StringSchalar) {
  ConfigValue config_value("zyxwvu");
  EXPECT_EQ(ConfigItem::kScalar, config_value.type());
  std::string value;
  EXPECT_TRUE(config_value.get(&value));
  EXPECT_STREQ("zyxwvu", value.c_str());
  config_value.set("abcdefg");
  EXPECT_TRUE(config_value.get(&value));
  EXPECT_STREQ("abcdefg", value.c_str());
}

TEST(RimeConfigItemTest, ConfigList) {
  ConfigList a;
  a.push_back(ConfigItemPtr(new ConfigItem));
  a.push_back(ConfigValue::Create(false));
  a.push_back(ConfigValue::Create(123));
  a.push_back(ConfigValue::Create(3.14));
  a.push_back(ConfigValue::Create("zyx"));
  a.push_back(ConfigList::Create());
  ASSERT_EQ(6, a.size());
  EXPECT_EQ(ConfigItem::kNull, a[0]->type());
  EXPECT_EQ(ConfigItem::kScalar, a[1]->type());
  EXPECT_EQ(ConfigItem::kList, a[5]->type());
  {
    bool value = true;
    EXPECT_FALSE(a[0]->get<bool>(&value));
    EXPECT_FALSE(a[2]->get<bool>(&value));
    EXPECT_FALSE(a[3]->get<bool>(&value));
    EXPECT_FALSE(a[4]->get<bool>(&value));
    EXPECT_FALSE(a[5]->get<bool>(&value));
    EXPECT_TRUE(a[1]->get<bool>(&value));
    EXPECT_FALSE(value);
  }
  {
    int value;
    EXPECT_TRUE(a[2]->get<int>(&value));
    EXPECT_EQ(123, value);
  }
  {
    double value;
    EXPECT_TRUE(a[3]->get<double>(&value));
    EXPECT_EQ(3.14, value);
  }
  {
    std::string value;
    EXPECT_TRUE(a[4]->get(&value));
    EXPECT_STREQ("zyx", value.c_str());
  }
  {
    shared_ptr<ConfigList> nested;
    nested = dynamic_pointer_cast<ConfigList>(a[5]);
    ASSERT_TRUE(nested);
    EXPECT_EQ(0, nested->size());
  }
}

TEST(RimeConfigItemTest, ConfigMap) {
  // TODO:
}

*/

TEST_F(RimeConfigTest, Config_IsNull) {
  bool is_null = config_->IsNull("terrans/tank");
  EXPECT_FALSE(is_null);

  is_null = config_->IsNull("protoss/tank");
  EXPECT_TRUE(is_null);
}

TEST_F(RimeConfigTest, Config_GetBool) {
  bool ret, value;
  ret = config_->GetBool("terrans/tank/seiged", &value);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(value);

  ret = config_->GetBool("zerg/lurker/burrowed", &value);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(value);
}

TEST_F(RimeConfigTest, Config_GetInt) {
  bool ret;
  int value;
  ret = config_->GetInt("terrans/supply/produced", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(28, value);

  ret = config_->GetInt("zerg/zergling/lost", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(1234, value);
}

TEST_F(RimeConfigTest, Config_GetDouble) {
  bool ret;
  double value;
  ret = config_->GetDouble("terrans/math/pi", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(3.1415926, value);

  ret = config_->GetDouble("protoss/battery/energy", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(10.111, value);
}

TEST_F(RimeConfigTest, Config_GetString) {
  bool ret;
  std::string value;
  ret = config_->GetString("protoss/residence", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("Aiur", value);

  ret = config_->GetString("zerg/queen", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("Kerrigan", value);
}

TEST_F(RimeConfigTest, Config_GetList) {
  shared_ptr<ConfigList> p;
  p = config_->GetList("protoss/air_force");
  ASSERT_TRUE(p);

  // TODO:
}

TEST_F(RimeConfigTest, Config_GetMap) {
  shared_ptr<ConfigMap> p;
  p = config_->GetMap("terrans/tank/cost");
  ASSERT_TRUE(p);

  // TODO:
}

