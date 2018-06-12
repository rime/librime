//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-20 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_NAVIGATOR_H_
#define RIME_NAVIGATOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>
#include <rime/gear/key_binding_processor.h>
#include <rime/gear/translator_commons.h>

namespace rime {

class Navigator : public Processor, public KeyBindingProcessor<Navigator> {
 public:
  explicit Navigator(const Ticket& ticket);

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;

  Handler Rewind;
  Handler LeftByChar;
  Handler RightByChar;
  Handler LeftBySyllable;
  Handler RightBySyllable;
  Handler Home;
  Handler End;

 private:
  void BeginMove(Context* ctx);
  bool JumpLeft(Context* ctx, size_t start_pos = 0);
  bool JumpRight(Context* ctx, size_t start_pos = 0);
  bool MoveLeft(Context* ctx);
  bool MoveRight(Context* ctx);
  bool GoHome(Context* ctx);
  bool GoToEnd(Context* ctx);

  string input_;
  Spans spans_;
};

}  // namespace rime

#endif  // RIME_NAVIGATOR_H_
