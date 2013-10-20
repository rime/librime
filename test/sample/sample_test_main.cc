#include <gtest/gtest.h>
#include <rime/setup.h>

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  rime::SetupLogging("rime.test");
  rime::LoadModules(rime::kDefaultModules);
  return RUN_ALL_TESTS();
}
