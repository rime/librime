// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-20 GONG Chen <chen.sst@gmail.com>
//
#include <rime/key_event.h>

namespace rime {

const std::string KeyEvent::repr() const {
  // first lookup predefined key name
  const char *name = GetKeyName(keycode_);
  if (name)
    return name;
  // no name :-| return its hex value
  char value[9] = {0};
  if (keycode_ <= 0xffff) {
    sprintf(value, "0x%4x", keycode_);
  }
  else if (keycode_ <= 0xffffff) {
    sprintf(value, "0x%6x", keycode_);
  }
  else {
    return "(unknown)";
  }
  return value;
}

}  // namespace rime
