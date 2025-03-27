#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime_api_impl.h>
#include <rime/service.h>
#include <rime/module.h>
#include <rime/setup.h>

class GlobalEnvironment : public testing::Environment {
 public:
  void SetUp() override {
    rime::SetupLogging("rime.test");

    RIME_STRUCT(RimeTraits, traits);
    // put all files in the working directory ($build/test).
    traits.shared_data_dir = traits.user_data_dir = traits.prebuilt_data_dir =
        traits.staging_dir = ".";

    RimeInitialize(&traits);
  }

  void TearDown() override { RimeFinalize(); }
};

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new GlobalEnvironment);

  return RUN_ALL_TESTS();
}
