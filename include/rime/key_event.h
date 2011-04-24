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

#include <iostream>
#include <string>
#include <vector>
#include <rime/common.h>
#include <rime/key_table.h>

namespace rime {

class KeyEvent {
 public:
  KeyEvent() : keycode_(0), modifier_(0) {}
  KeyEvent(int keycode, int modifier)
      : keycode_(keycode), modifier_(modifier) {}
  KeyEvent(const std::string &repr);
  
  int keycode() const { return keycode_; }
  void keycode(int value) { keycode_ = value; }
  int modifier() const { return modifier_; }
  void modifier(int value) { modifier_ = value; }

  bool shift() const { return (modifier_ & kShiftMask) != 0; }
  bool ctrl() const { return (modifier_ & kControlMask) != 0; }
  bool alt() const { return (modifier_ & kAltMask) != 0; }
  bool release() const { return (modifier_ & kReleaseMask) != 0; }
  // 按鍵表示為形如「狀態+鍵名」的文字
  // 若無鍵名，則以四位或六位十六进制数形式的文字來標識
  // 形如 "0x12ab", "0xfffffe"
  const std::string repr() const;

  // 解析文字表示的按鍵
  bool Parse(const std::string &repr);

  bool operator== (const KeyEvent &other) {
    return keycode_ == other.keycode_ && modifier_ == other.modifier_;
  }

 private:
  int keycode_;
  int modifier_;
};

// 按鍵序列
class KeySequence : public std::vector<KeyEvent> {
 public:
  KeySequence() {}
  KeySequence(const std::string &repr);

  // 可表示為一串文字
  // 若其中包含不產生可打印字符的按鍵，以 {鍵名} 來標記
  // 組合鍵也用 {組合鍵狀態+鍵名} 來標記
  const std::string repr() const;

  // 解析按鍵序列描述文字
  bool Parse(const std::string &repr);
};

inline std::ostream& operator<< (std::ostream& out, const KeyEvent &key_event) {
  out << key_event.repr();
  return out;
}

inline std::ostream& operator<< (std::ostream& out, const KeySequence &key_seq) {
  out << key_seq.repr();
  return out;
}

}  // namespace rime

#endif  // RIME_KEY_EVENT_H_
