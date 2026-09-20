//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_KEY_BINDER_H_
#define RIME_KEY_BINDER_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

struct KeyBinding;
class KeyBindings;

class KeyBinder : public Processor {
 public:
  KeyBinder(const Ticket& ticket);
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  void LoadConfig();
  bool ReinterpretPagingKey(const KeyEvent& key_event);
  void PerformKeyBinding(const KeyBinding& binding);

  the<KeyBindings> key_bindings_;
  bool redirecting_ = false;
  // 記錄上一記「真正執行了翻頁」的按鍵鍵碼（0 表示無待重釋鍵）
  int last_paging_key_ = 0;
  int paging_keystroke_count_ = 0;
};

}  // namespace rime

#endif  // RIME_KEY_BINDER_H_
