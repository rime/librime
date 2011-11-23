// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-20 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_NAVIGATOR_H_
#define RIME_NAVIGATOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Navigator : public Processor {
 public:
  Navigator(Engine *engine) : Processor(engine) {}
  virtual ~Navigator() {}
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);

 private:
  bool Left(Context *ctx);
  bool Right(Context *ctx);
  bool Home(Context *ctx);
  bool End(Context *ctx);
};

}  // namespace rime

#endif  // RIME_NAVIGATOR_H_
