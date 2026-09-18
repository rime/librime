//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <memory>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/processor.h>
#include <rime/schema.h>
#include <rime/ticket.h>

using namespace rime;

namespace {

struct ModifierKeyPair {
  const char* name;
  const char* bound_key_name;
  int other_key;
  int bound_key;
  int modifier;
};

class AsciiComposerModifierKeyTest
    : public ::testing::TestWithParam<ModifierKeyPair> {
 protected:
  void SetUp() override {
    const auto& keys = GetParam();
    auto config = new Config;
    ASSERT_TRUE(config->SetItem("ascii_composer/good_old_caps_lock",
                                New<ConfigValue>(false)));
    ASSERT_TRUE(config->SetString(
        string("ascii_composer/switch_key/") + keys.bound_key_name,
        "set_ascii_mode"));
    engine_.reset(Engine::Create());
    engine_->ApplySchema(new Schema("ascii_composer_test", config));
    auto component = Processor::Require("ascii_composer");
    ASSERT_NE(nullptr, component);
    composer_.reset(component->Create(Ticket(engine_.get(), "ascii_composer")));
    ASSERT_NE(nullptr, composer_);
  }

  the<Engine> engine_;
  the<Processor> composer_;
};

TEST_P(AsciiComposerModifierKeyTest, IgnoresReleaseFromAnotherPhysicalKey) {
  const auto& keys = GetParam();
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.other_key, keys.modifier)));
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier | kReleaseMask)));
  EXPECT_FALSE(engine_->context()->get_option("ascii_mode"));
}

TEST_P(AsciiComposerModifierKeyTest, TogglesOnMatchingRelease) {
  const auto& keys = GetParam();
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier)));
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier | kReleaseMask)));
  EXPECT_TRUE(engine_->context()->get_option("ascii_mode"));
}

TEST_P(AsciiComposerModifierKeyTest, IgnoresSecondPhysicalModifierKey) {
  const auto& keys = GetParam();
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.other_key, keys.modifier)));
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier)));
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier | kReleaseMask)));
  EXPECT_FALSE(engine_->context()->get_option("ascii_mode"));
}

TEST_P(AsciiComposerModifierKeyTest, IgnoresModifierUsedWithAnotherKey) {
  const auto& keys = GetParam();
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier)));
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(KeyEvent('A', keys.modifier)));
  EXPECT_EQ(kNoop, composer_->ProcessKeyEvent(
                       KeyEvent(keys.bound_key, keys.modifier | kReleaseMask)));
  EXPECT_FALSE(engine_->context()->get_option("ascii_mode"));
}

INSTANTIATE_TEST_SUITE_P(
    ModifierKeys,
    AsciiComposerModifierKeyTest,
    ::testing::Values(
        ModifierKeyPair{"Shift", "Shift_L", XK_Shift_R, XK_Shift_L, kShiftMask},
        ModifierKeyPair{"Control", "Control_L", XK_Control_R, XK_Control_L,
                        kControlMask},
        ModifierKeyPair{"Alt", "Alt_L", XK_Alt_R, XK_Alt_L, kAltMask},
        ModifierKeyPair{"Super", "Super_L", XK_Super_R, XK_Super_L,
                        kSuperMask}),
    [](const ::testing::TestParamInfo<ModifierKeyPair>& info) {
      return info.param.name;
    });

}  // namespace
