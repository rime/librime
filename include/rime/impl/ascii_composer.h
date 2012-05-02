// vim: set sts=2 sw=2 et:
// encoding: utf-8
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
  kAsciiModeSwitchInline,
  kAsciiModeSwitchCommitText,
  kAsciiModeSwitchCommitCode,
  kAsciiModeSwitchNoop
};
  
typedef std::map<int /* keycode */,
                 AsciiModeSwitchStyle> AsciiModeSwitchKeyBindings;

class AsciiComposer : public Processor {
 public:
  AsciiComposer(Engine *engine);
  virtual ~AsciiComposer() {}
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
  
 protected:
  void LoadConfig(Schema* schema);
  void ToggleAsciiMode(int key_code);
  void OnContextUpdate(Context *ctx);

  AsciiModeSwitchKeyBindings bindings_;
  bool shift_key_pressed_;
  bool ctrl_key_pressed_;
  boost::signals::connection connection_;
};

}  // namespace rime

#endif  // RIME_ASCII_COMPOSER_H_
