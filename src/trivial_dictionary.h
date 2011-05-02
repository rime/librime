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

#include <rime/basic_dictionary.h>

namespace rime {
  
class TrivialDictionary : public BasicDictionary {
  public:
    virtual ~TrivialDictionary(){};
    virtual void Lookup(const Engine *engine, DictionaryResult& dict_result); 
  private:
    
};

} //  namespace rime

#endif  //  TRIVIAL_DICTIONARY_H_


