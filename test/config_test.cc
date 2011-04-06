// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#include "../src/config.h"

#include <rime/common.h>
#include <rime/component.h>
#include <gtest/gtest.h>


using namespace rime;

TEST(RimeConfigTest, ConfigCreation) {
  ConfigClass* config_klass(new ConfigClass);
  EXPECT_STREQ("config", config_klass->name().c_str());
  config_klass->Register();
  shared_ptr<Config> config = dynamic_pointer_cast<Config>(Component::Create("config", NULL));
  EXPECT_TRUE(config);
}
