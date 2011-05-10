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
#include <rime/schema.h>

namespace rime {

class Context;
class Schema;

class Dictionary : public Class<Dictionary, const Schema*> {
 public:
  Dictionary(){};
  Dictionary(const Schema *schema) : schema_(schema) {}
  virtual ~Dictionary() {}

  virtual void Lookup(Context *context, DictionaryResult *dict_result) = 0;

 protected:
  const Schema *schema_;
};

}  // namespace rime

#endif  // DICTIONARY_H_


