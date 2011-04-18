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
#include "../src/yaml_config.h"

using namespace rime;

class RimeConfigTest : public ::testing::Test {
 protected:
  RimeConfigTest() : component_(NULL), config_(NULL) {}

  virtual void SetUp() {
    component_ = new YamlConfigComponent(".");
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

TEST_F(RimeConfigTest, ConfigCreation) {
  // registration
  Component::Register("test_config", new YamlConfigComponent("."));
  // finding component
  Config::Component *cc = Config::Find("test_config");
  // creation
  scoped_ptr<Config> config(cc->Create("test.yaml"));
  EXPECT_TRUE(config);
}

TEST_F(RimeConfigTest, Config_IsNull) {
  bool is_null = config_->IsNull("root/bool");
  EXPECT_TRUE(is_null);

  is_null = config_->IsNull("toor/loob");
  EXPECT_FALSE(is_null);
}

TEST_F(RimeConfigTest, Config_GetBool) {
  bool ret, value;
  ret = config_->GetBool("root/bool", &value);
  EXPECT_TRUE(ret);
  EXPECT_FALSE(value);

  ret = config_->GetBool("root2/high/bool", &value);
  EXPECT_TRUE(ret);
  EXPECT_TRUE(value);
}

TEST_F(RimeConfigTest, Config_GetInt) {
  bool ret;
  int value;
  ret = config_->GetInt("root/int", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(1234, value);

  ret = config_->GetInt("root2/mid/int", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(28, value);
}

TEST_F(RimeConfigTest, Config_GetDouble) {
  bool ret;
  double value;
  ret = config_->GetDouble("root/double", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(3.1415926, value);

  ret = config_->GetDouble("root2/low/double", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ(10.111, value);
}

TEST_F(RimeConfigTest, Config_GetString) {
  bool ret;
  std::string value;
  ret = config_->GetString("root/string", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("IOU", value);

  ret = config_->GetString("root2/low/string", &value);
  EXPECT_TRUE(ret);
  EXPECT_EQ("ABC", value);
}
