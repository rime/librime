#include <gtest/gtest.h>
#include <rime/setup.h>
#include <rime/service.h>

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  rime::SetupLogging("rime.test");
  rime::Service::instance().deployer().staging_dir = ".";
  rime::LoadModules(rime::kDefaultModules);
  return RUN_ALL_TESTS();
}
