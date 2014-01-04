#include <gtest/gtest.h>
#include <rime/setup.h>

static const char* test_modules[] = { "core", "sample", NULL };

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  rime::SetupLogging("sample.test");
  rime::LoadModules(test_modules);
  return RUN_ALL_TESTS();
}
