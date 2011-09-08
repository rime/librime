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
  enum Result {
    kRejected,  // do the OS default processing
    kAccepted,  // consume it
    kNoop,      // leave it to other processors
  };
  explicit Processor(Engine *engine) : engine_(engine) {}
  virtual ~Processor() {}

  virtual Result ProcessKeyEvent(const KeyEvent &/*key_event*/) {
    return kNoop;
  }

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // RIME_PROCESSOR_H_
