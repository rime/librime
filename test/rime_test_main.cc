#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime/service.h>
#include <rime/module.h>
#include <rime/setup.h>

class GlobalEnvironment : public testing::Environment {
  public:
    void TearDown() override {
      rime::Service::instance().deployer().JoinMaintenanceThread();
      rime::Service::instance().StopService();
      rime::Registry::instance().Clear();
      rime::ModuleManager::instance().UnloadModules();
    }
  };

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new GlobalEnvironment);

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
