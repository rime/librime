// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-06 Zou xu <zouivex@gmail.com>
// 2011-04-08 GONG Chen <chen.sst@gmail.com>
//

#include <gtest/gtest.h>
#include <rime/component.h>
#include <rime/config.h>
#include "../src/yaml_config.h"

using namespace rime;

Config::Component *cc;
Config* config;

TEST(RimeConfigTest, ConfigCreation) {
  // registration
  Component::Register("test_config", new YamlConfigComponent("."));
  // finding component
  cc = Config::Find("test_config");
  // creation
  config = cc->Create("test.yaml");
  EXPECT_TRUE(config);
}

TEST(RimeConfigTest, Config_IsNull) {
  bool bNull = config->IsNull("root/bool");
  EXPECT_TRUE(bNull);

  bNull = config->IsNull("toor/loob");
  EXPECT_FALSE(bNull);
}

TEST(RimeConfigTest, Config_GetBool) {
  bool bRet, bValue;
  bRet = config->GetBool("root/bool", &bValue);
  EXPECT_TRUE(bRet);
  EXPECT_FALSE(bValue);

  bRet = config->GetBool("root2/high/bool", &bValue);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(bValue);
}

TEST(RimeConfigTest, Config_GetInt) {
  bool bRet;
  int value;
  bRet = config->GetInt("root/int", &value);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(value == 1234);

  bRet = config->GetInt("root2/mid/int", &value);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(value == 28);
}

TEST(RimeConfigTest, Config_GetDouble) {
  bool bRet;
  double value;
  bRet = config->GetDouble("root/double", &value);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(value==3.1415926);

  bRet = config->GetDouble("root2/low/double", &value);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(value==10.111);
}

TEST(RimeConfigTest, Config_GetString) {
  bool bRet;
  std::string value;
  bRet = config->GetString("root/string", &value);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(value=="IOU");

  bRet = config->GetString("root2/low/string", &value);
  EXPECT_TRUE(bRet);
  EXPECT_TRUE(value=="ABC");
}
