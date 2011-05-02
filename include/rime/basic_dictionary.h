// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef BASIC_DICTIONARY_H_
#define BASIC_DICTIONARY_H_

#include <rime/engine.h>
#include <rime/dictionary_result.h>

namespace rime {
  
class BasicDictionary {
  public:
    // Lookup
    // @param engine : 
    // @param dict_result :
    virtual void Lookup(const Engine *engine, DictionaryResult& dict_result);
    virtual ~BasicDictionary(){};
  private:
    //
};

} //  namespace rime

#endif  //  BASIC_DICTIONARY_H_


