// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
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
  Selector(Engine *engine) : Processor(engine) {}
  virtual ~Selector() {}
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
};

}  // namespace rime

#endif  // RIME_SELECTOR_H_
