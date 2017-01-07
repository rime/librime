//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-09-11 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SELECTOR_H_
#define RIME_SELECTOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Selector : public Processor {
 public:
  Selector(const Ticket& ticket);

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  bool PageUp(Context* ctx);
  bool PageDown(Context* ctx);
  bool CursorUp(Context* ctx);
  bool CursorDown(Context* ctx);
  bool Home(Context* ctx);
  bool End(Context* ctx);
  bool SelectCandidateAt(Context* ctx, int index);
};

}  // namespace rime

#endif  // RIME_SELECTOR_H_
