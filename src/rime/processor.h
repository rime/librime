//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_PROCESSOR_H_
#define RIME_PROCESSOR_H_

#include <rime/component.h>
#include <rime/ticket.h>

namespace rime {

class Engine;
class KeyEvent;

enum ProcessResult {
  kRejected,  // do the OS default processing
  kAccepted,  // consume it
  kNoop,      // leave it to other processors
};

class Processor : public Class<Processor, const Ticket&> {
 public:
  explicit Processor(const Ticket& ticket)
      : engine_(ticket.engine), name_space_(ticket.name_space) {}
  virtual ~Processor() = default;

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event) {
    return kNoop;
  }

 protected:
  Engine* engine_;
  string name_space_;
};

}  // namespace rime

#endif  // RIME_PROCESSOR_H_
