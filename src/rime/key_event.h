﻿// encoding: utf-8
//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_KEY_EVENT_H_
#define RIME_KEY_EVENT_H_

#include <iostream>
#include <rime/common.h>
#include <rime/key_table.h>

namespace rime {

class KeyEvent {
 public:
  KeyEvent() = default;
  KeyEvent(int keycode, int modifier)
      : keycode_(keycode), modifier_(modifier) {}
  RIME_API KeyEvent(const string& repr);

  int keycode() const { return keycode_; }
  void keycode(int value) { keycode_ = value; }
  int modifier() const { return modifier_; }
  void modifier(int value) { modifier_ = value; }

  bool shift() const { return (modifier_ & kShiftMask) != 0; }
  bool ctrl() const { return (modifier_ & kControlMask) != 0; }
  bool alt() const { return (modifier_ & kAltMask) != 0; }
  bool caps() const { return (modifier_ & kLockMask) != 0; }
  bool super() const { return (modifier_ & kSuperMask) != 0; }
  bool release() const { return (modifier_ & kReleaseMask) != 0; }
  // 按鍵表示為形如「狀態+鍵名」的文字
  // 若無鍵名，則以四位或六位十六进制数形式的文字來標識
  // 形如 "0x12ab", "0xfffffe"
  RIME_API string repr() const;

  // 解析文字表示的按鍵
  RIME_API bool Parse(const string& repr);

  bool operator== (const KeyEvent& other) const {
    return keycode_ == other.keycode_ && modifier_ == other.modifier_;
  }

  bool operator< (const KeyEvent& other) const {
    if (keycode_ != other.keycode_)
      return keycode_ < other.keycode_;
    return modifier_ < other.modifier_;
  }

 private:
  int keycode_ = 0;
  int modifier_ = 0;
};

// 按鍵序列
class KeySequence : public vector<KeyEvent> {
 public:
  KeySequence() = default;
  RIME_API KeySequence(const string& repr);

  // 可表示為一串文字
  // 若其中包含不產生可打印字符的按鍵，以 {鍵名} 來標記
  // 組合鍵也用 {組合鍵狀態+鍵名} 來標記
  RIME_API string repr() const;

  // 解析按鍵序列描述文字
  RIME_API bool Parse(const string& repr);
};

inline std::ostream& operator<< (std::ostream& out, const KeyEvent& key_event) {
  out << key_event.repr();
  return out;
}

inline std::ostream& operator<< (std::ostream& out, const KeySequence& key_seq) {
  out << key_seq.repr();
  return out;
}

}  // namespace rime

#endif  // RIME_KEY_EVENT_H_
