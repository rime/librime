// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TRIVIAL_PROCESSOR_H_
#define RIME_TRIVIAL_PROCESSOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {  

class TrivialProcessor : public Processor {
 public:
  TrivialProcessor(Engine *engine) : Processor(engine) {}
  virtual ~TrivialProcessor() {}
  virtual bool ProcessKeyEvent(const KeyEvent &key_event);
};

class TrivialProcessorComponent : public Component<TrivialProcessor> {
};

}  // namespace rime

#endif  // RIME_TRIVIAL_PROCESSOR_H_
