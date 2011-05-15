// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <rime/component.h>
#include <rime/dictionary_result.h>

namespace rime {

class Context;
class Engine;

class Dictionary : public Class<Dictionary, Engine*> {
 public:
  Dictionary(Engine *engine) : engine_(engine) {}
  virtual ~Dictionary() {}

  virtual void Lookup(Context *context, DictionaryResult *dict_result) = 0;

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // DICTIONARY_H_


