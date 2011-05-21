// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATOR_H_
#define RIME_TRANSLATOR_H_

#include <rime/component.h>
#include <rime/translation.h>

namespace rime {

class Context;
class Engine;

class Translator : public Class<Translator, Engine*> {
 public:
  Translator(Engine *engine) : engine_(engine) {}
  virtual ~Translator() {}

  virtual void Query(Context *context, Translation *translation) = 0;

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // RIME_TRANSLATOR_H_


