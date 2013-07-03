//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-07-02 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_FORMATTER_H_
#define RIME_FORMATTER_H_

#include <string>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

class Engine;

class Formatter : public Class<Formatter, Engine*> {
 public:
  Formatter(Engine *engine) : engine_(engine) {}
  virtual ~Formatter() {}

  virtual void Format(std::string* text) = 0;

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // RIME_FORMATTER_H_
