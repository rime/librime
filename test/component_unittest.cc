#include <gtest/gtest.h>
#include <rime/component.h>

using namespace rime;

TEST(RimeComponentTest, ComponentCreation) {
  EXPECT_FALSE(Component::Create("unknown", NULL));
}
