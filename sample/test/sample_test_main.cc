#include <gtest/gtest.h>
#include <rime/setup.h>

static RIME_MODULE_LIST(test_modules, "sample");

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  rime::SetupLogging("sample.test");
  rime::LoadModules(test_modules);
  return RUN_ALL_TESTS();
}
