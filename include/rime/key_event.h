// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_KEY_EVENT_H_
#define RIME_KEY_EVENT_H_

#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

class KeyEvent {
 public:
  KeyEvent() : keycode_(0), modifier_(0) {}
  KeyEvent(int keycode, int modifier)
      : keycode_(keycode), modifier_(modifier) {}
  
  int keycode() const { return keycode_; }
  void keycode(int value) { keycode_ = value; }
  int modifier() const { return modifier_; }
  void modifier(int value) { modifier_ = value; }

  const std::string ToString() const;
  void FromString(const std::string &repr);

 private:
  int keycode_;
  int modifier_;
};

class KeySequence : public std::vector<KeyEvent> {
 public:
  KeySequence() {}
  KeySequence(const std::string &repr) {
    FromString(repr);
  }
  const std::string ToString() const;
  void FromString(const std::string &repr);
};

}  // namespace rime

#endif  // RIME_KEY_EVENT_H_
