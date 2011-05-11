// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-04-26 Wensong He <snowhws@gmail.com>
//
#ifndef TRIVIAL_DICTIONARY_H_
#define TRIVIAL_DICTIONARY_H_

#include <rime/dictionary.h>

namespace rime {
  
class TrivialDictionary : public Dictionary {
 public:
  TrivialDictionary(){};
  TrivialDictionary(const Schema *schema) : Dictionary(schema) {}
  virtual ~TrivialDictionary() {}
  virtual void Lookup(Context *context, DictionaryResult *dict_result);
 private:
  
};

}  // namespace rime

#endif  // TRIVIAL_DICTIONARY_H_


