// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_PROCESSOR_H_
#define RIME_PROCESSOR_H_

#include <rime/component.h>

namespace rime {

class Engine;
class KeyEvent;

class Processor : public Class<Processor, Engine*> {
 public:
 public:
  explicit Processor(Engine *engine) : engine_(engine) {}
  virtual ~Processor() {}

  virtual bool ProcessKeyEvent(const KeyEvent &/*key_event*/) {
    return false;
  }

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // RIME_PROCESSOR_H_
