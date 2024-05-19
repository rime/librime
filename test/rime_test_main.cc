#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime/service.h>
#include <rime/setup.h>

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  RIME_STRUCT(RimeTraits, traits);
  // put all files in the working directory ($build/test).
  traits.shared_data_dir = traits.user_data_dir = traits.prebuilt_data_dir =
      traits.staging_dir = ".";
  rime::SetupDeployer(&traits);
  rime::SetupLogging("rime.test");
  rime::LoadModules(rime::kDefaultModules);
  rime::Service::instance().StartService();

  return RUN_ALL_TESTS();
}
