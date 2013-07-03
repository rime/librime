//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-07-02 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_SHAPE_H_
#define RIME_SHAPE_H_

#include <rime/formatter.h>
#include <rime/processor.h>

namespace rime {

class ShapeFormatter : public Formatter {
 public:
  ShapeFormatter(Engine *engine) : Formatter(engine) {
  }
  virtual void Format(std::string* text);
};

class ShapeProcessor : public Processor {
 public:
  ShapeProcessor(Engine *engine) : Processor(engine),
                                   formatter_(engine) {
  }
  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 private:
  ShapeFormatter formatter_;
};

}  // namespace rime

#endif  // RIME_FORMATTER_H_
