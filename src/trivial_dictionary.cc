// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#include "trivial_dictionary.h"

namespace rime{

void TrivialDictionary::Lookup(const Engine *engine, DictionaryResult& dict_result){
  dict_result.set_result(engine->context()->input());
}

}  // namespace rime
