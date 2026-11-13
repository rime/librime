//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <gtest/gtest.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/setup.h>

using namespace rime;

class AsciiComposerCapsLockTest : public ::testing::Test {
 protected:
  void SetUp() override {
    SetupLogging("rime.test");
    engine_.reset(Engine::Create());
    engine_->sink().connect([this](const string& text) { committed_ += text; });
  }

  void TearDown() override { engine_.reset(); }

  void Configure(bool off_is_ascii, bool good_old, bool with_editing = false) {
    auto* schema = engine_->schema();
    auto* config = schema->config();
    config->SetString("schema/schema_id", "test_ascii_composer");
    config->SetString("ascii_composer/switch_key/Caps_Lock", "commit_code");
    config->SetBool("ascii_composer/caps_lock_off_is_ascii", off_is_ascii);
    config->SetBool("ascii_composer/good_old_caps_lock", good_old);
    auto processors = New<ConfigList>();
    processors->Append(New<ConfigValue>("ascii_composer"));
    if (with_editing) {
      processors->Append(New<ConfigValue>("speller"));
      processors->Append(New<ConfigValue>("fluid_editor"));
    }
    config->SetItem("engine/processors", processors);
    engine_->ApplySchema(schema);
  }

  // Caps_Lock key as seen by IBus: the modifier reflects the state before
  // the key takes effect.
  void PressCapsLock(bool light_was_on) {
    engine_->ProcessKey(KeyEvent(XK_Caps_Lock, light_was_on ? kLockMask : 0));
  }

  bool ascii_mode() { return engine_->context()->get_option("ascii_mode"); }

  the<Engine> engine_;
  string committed_;
};

TEST_F(AsciiComposerCapsLockTest, DefaultLightOnIsAscii) {
  Configure(false, false);
  EXPECT_FALSE(ascii_mode());
  PressCapsLock(false);  // light turns on
  EXPECT_TRUE(ascii_mode());
  PressCapsLock(true);  // light turns off
  EXPECT_FALSE(ascii_mode());
}

TEST_F(AsciiComposerCapsLockTest, DefaultLightOnFlipsLetterCase) {
  Configure(false, false);
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('A', kLockMask)));
  EXPECT_EQ(committed_, "a");
  committed_.clear();
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('a', kLockMask | kShiftMask)));
  EXPECT_EQ(committed_, "A");
}

TEST_F(AsciiComposerCapsLockTest, LightOffIsAscii) {
  Configure(true, false);
  EXPECT_FALSE(ascii_mode());
  PressCapsLock(false);  // light turns on
  EXPECT_FALSE(ascii_mode());
  PressCapsLock(true);  // light turns off
  EXPECT_TRUE(ascii_mode());
  PressCapsLock(false);
  EXPECT_FALSE(ascii_mode());
}

TEST_F(AsciiComposerCapsLockTest, LightOnLetterCaseIsFlippedBack) {
  Configure(true, false);
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('A', kLockMask)));
  EXPECT_EQ(committed_, "a");
  committed_.clear();
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('a', kLockMask | kShiftMask)));
  EXPECT_EQ(committed_, "A");
}

TEST_F(AsciiComposerCapsLockTest, LightOnLetterReachesLaterProcessors) {
  Configure(true, false, true);
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('A', kLockMask)));
  EXPECT_EQ(engine_->context()->input(), "a");
  EXPECT_TRUE(committed_.empty());
}

TEST_F(AsciiComposerCapsLockTest, LightOnEditingKeysWork) {
  Configure(true, false, true);
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('A', kLockMask)));
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('B', kLockMask)));
  EXPECT_EQ(engine_->context()->input(), "ab");
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_BackSpace, kLockMask)));
  EXPECT_EQ(engine_->context()->input(), "a");
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent('C', kLockMask)));
  EXPECT_EQ(engine_->context()->input(), "ac");
  EXPECT_TRUE(engine_->ProcessKey(KeyEvent(XK_Return, kLockMask)));
  EXPECT_EQ(committed_, "ac");
}

TEST_F(AsciiComposerCapsLockTest, LightOnNonLettersAreNotIntercepted) {
  Configure(true, false);
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent(XK_space, kLockMask)));
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent('1', kLockMask)));
  EXPECT_FALSE(engine_->ProcessKey(KeyEvent('A', kLockMask | kReleaseMask)));
  EXPECT_TRUE(committed_.empty());
}

TEST_F(AsciiComposerCapsLockTest, GoodOldCapsLockTakesPrecedence) {
  Configure(true, true);
  PressCapsLock(false);
  EXPECT_TRUE(ascii_mode());
  PressCapsLock(true);
  EXPECT_FALSE(ascii_mode());
}
