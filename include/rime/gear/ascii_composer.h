//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ASCII_COMPOSER_H_
#define RIME_ASCII_COMPOSER_H_

#include <map>
#include <boost/signals.hpp>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

enum AsciiModeSwitchStyle {
  kAsciiModeSwitchNoop,
  kAsciiModeSwitchInline,
  kAsciiModeSwitchCommitText,
  kAsciiModeSwitchCommitCode,
};

typedef std::map<int /* keycode */,
                 AsciiModeSwitchStyle> AsciiModeSwitchKeyBindings;

class AsciiComposer : public Processor {
 public:
  AsciiComposer(Engine *engine);
  virtual ~AsciiComposer() {}
  virtual Result ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  Result ProcessCapsLock(const KeyEvent& key_event);
  void LoadConfig(Schema* schema);
  bool ToggleAsciiModeWithKey(int key_code);
  void SwitchAsciiMode(bool ascii_mode, AsciiModeSwitchStyle style);
  void OnContextUpdate(Context *ctx);

  // config options
  AsciiModeSwitchKeyBindings bindings_;
  AsciiModeSwitchStyle caps_lock_switch_style_;
  // state
  bool shift_key_pressed_;
  bool ctrl_key_pressed_;
  boost::signals::connection connection_;
};

}  // namespace rime

#endif  // RIME_ASCII_COMPOSER_H_
