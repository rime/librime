#include <gtest/gtest.h>
#include <rime_api.h>
#include <rime/service.h>
#include <rime/setup.h>

class GlobalEnvironment : public testing::Environment {
 public:
  void SetUp() override {
    RIME_STRUCT(RimeTraits, traits);
    // put all files in the working directory ($build/test).
    traits.shared_data_dir = traits.user_data_dir = traits.prebuilt_data_dir =
        traits.staging_dir = ".";
    // for logging.
    traits.app_name = "rime.test";
    rime_get_api()->setup(&traits);
    rime_get_api()->initialize(&traits);
  }

  void TearDown() override { rime_get_api()->finalize(); }
};

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new GlobalEnvironment);

  return RUN_ALL_TESTS();
}
