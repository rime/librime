// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_FLUENCY_EDITOR_H_
#define RIME_FLUENCY_EDITOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class FluencyEditor : public Processor {
 public:
  FluencyEditor(Engine *engine) : Processor(engine) {}
  virtual ~FluencyEditor() {}
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
};

}  // namespace rime

#endif  // RIME_FLUENCY_EDITOR_H_
