// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#include <rime/common.h>
#include <rime/component.h>
#include <gtest/gtest.h>
#include <rime/ConfigComponent.h>

using namespace rime;

TEST(RimeConfigComponentTest, ConfigComponentCreation) {
  shared_ptr<ConfigComponentClass> config_klass(new ConfigComponentClass);
  EXPECT_STREQ("ConfigComponent", config_klass->name().c_str());
  config_klass->Register();
  shared_ptr<ConfigComponent> configComponent = dynamic_pointer_cast<ConfigComponent>(Component::Create("ConfigComponent", NULL));
  EXPECT_TRUE(configComponent);
}
