#ifndef RIME_ASCII_COMPOSER_H_
#define RIME_ASCII_COMPOSER_H_

#include <chrono>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/key_event.h>
#include <rime/processor.h>
#include <rime/gear/key_binding_processor.h>

namespace rime {

class Context;
class Schema;

enum AsciiModeSwitchStyle {
  kAsciiModeSwitchNoop,
  kAsciiModeSwitchInline,
  kAsciiModeSwitchCommitText,
  kAsciiModeSwitchCommitCode,
  kAsciiModeSwitchClear,
  kAsciiModeSet,
  kAsciiModeUnset,
};

using AsciiModeSwitchKeyBindings = map<int /* keycode */, AsciiModeSwitchStyle>;

class AsciiComposer : public Processor,
                      public KeyBindingProcessor<AsciiComposer> {
 public:
  AsciiComposer(const Ticket& ticket);
  virtual ~AsciiComposer();

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

  // 開放給 bindings 的動作處理函數
  bool CommitRawInput(Context* ctx);
  bool CommitRawInputWithSpace(Context* ctx);

 protected:
  void LoadConfig(Schema* schema);
  bool ToggleAsciiModeWithKey(int key_code);
  void SwitchAsciiMode(bool ascii_mode, AsciiModeSwitchStyle style);
  void TransitInlineAscii(Context* ctx, bool entering_ascii);
  ProcessResult ProcessCapsLock(const KeyEvent& key_event);
  void OnContextUpdate(Context* ctx);
  void OnCommit(Context* ctx);

 private:
  void ResetModifierState();
  void CommitAndReset(const string& text);
  void EnterInlineAsciiWithChar(char ch);

  AsciiModeSwitchKeyBindings bindings_;
  AsciiModeSwitchStyle caps_lock_switch_style_ = kAsciiModeSwitchNoop;
  bool good_old_caps_lock_ = false;
  bool inline_uppercase_ = false;
  bool inline_keypad_ = false;
  bool toggle_with_caps_ = false;

  bool shift_key_pressed_ = false;
  bool ctrl_key_pressed_ = false;
  bool alt_key_pressed_ = false;
  bool super_key_pressed_ = false;
  std::chrono::steady_clock::time_point toggle_expired_;

  // 實體按鍵總賬與快照
  string raw_keys_;
  string cached_input_;
  string cached_raw_keys_;

  bool is_inline_ascii_ = false;  // 當前是否處於「臨時西文」軌道
  connection update_connection_;
  connection commit_connection_;
};

}  // namespace rime

#endif  // RIME_ASCII_COMPOSER_H_
