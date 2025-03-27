#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime/service.h>
#include <rime/setup.h>

#ifndef _WIN32
// this include breaks building on Windows:
// src\rime_api_impl.h:25:22: error: dllimport cannot be applied to non-inline
// function definition 25 | RIME_DEPRECATED void RimeSetup(RimeTraits* traits)
#include <rime_api_impl.h>
#endif

class GlobalEnvironment : public testing::Environment {
 public:
  void SetUp() override {
    rime::SetupLogging("rime.test");

    RIME_STRUCT(RimeTraits, traits);
    // put all files in the working directory ($build/test).
    traits.shared_data_dir = traits.user_data_dir = traits.prebuilt_data_dir =
        traits.staging_dir = ".";

#ifdef _WIN32
    rime::SetupDeployer(&traits);
    rime::LoadModules(rime::kDefaultModules);
    rime::Service::instance().StartService();
  }
#else
    // not working on Windows since <rime_api_impl.h> is not included
    RimeInitialize(&traits);
  }

  void TearDown() override { RimeFinalize(); }
#endif
};

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new GlobalEnvironment);

  return RUN_ALL_TESTS();
}
