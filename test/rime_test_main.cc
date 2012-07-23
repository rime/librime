#include <gtest/gtest.h>
#include <rime/common.h>

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  rime::RegisterComponents();
  return RUN_ALL_TESTS();
}
