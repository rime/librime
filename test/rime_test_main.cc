#include <gtest/gtest.h>
#include <rime/common.h>

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  rime::RegisterComponents();
  return RUN_ALL_TESTS();
}
