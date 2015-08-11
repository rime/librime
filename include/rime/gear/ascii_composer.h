//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ASCII_COMPOSER_H_
#define RIME_ASCII_COMPOSER_H_

#include <chrono>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Context;
class Schema;

enum AsciiModeSwitchStyle {
  kAsciiModeSwitchNoop,
  kAsciiModeSwitchInline,
  kAsciiModeSwitchCommitText,
  kAsciiModeSwitchCommitCode,
  kAsciiModeSwitchClear,
};

using AsciiModeSwitchKeyBindings = map<int /* keycode */,
                                            AsciiModeSwitchStyle>;

class AsciiComposer : public Processor {
 public:
  AsciiComposer(const Ticket& ticket);
  ~AsciiComposer();

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  ProcessResult ProcessCapsLock(const KeyEvent& key_event);
  void LoadConfig(Schema* schema);
  bool ToggleAsciiModeWithKey(int key_code);
  void SwitchAsciiMode(bool ascii_mode, AsciiModeSwitchStyle style);
  void OnContextUpdate(Context* ctx);

  // config options
  AsciiModeSwitchKeyBindings bindings_;
  AsciiModeSwitchStyle caps_lock_switch_style_ = kAsciiModeSwitchNoop;
  bool good_old_caps_lock_ = false;
  // state
  bool toggle_with_caps_ = false;
  bool shift_key_pressed_ = false;
  bool ctrl_key_pressed_ = false;
  using TimePoint = std::chrono::steady_clock::time_point;
  TimePoint toggle_expired_;
  connection connection_;
};

}  // namespace rime

#endif  // RIME_ASCII_COMPOSER_H_
