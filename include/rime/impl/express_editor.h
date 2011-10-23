// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_EXPRESS_EDITOR_H_
#define RIME_EXPRESS_EDITOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class ExpressEditor : public Processor {
 public:
  ExpressEditor(Engine *engine) : Processor(engine) {}
  virtual ~ExpressEditor() {}
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
};

}  // namespace rime

#endif  // RIME_EXPRESS_EDITOR_H_
