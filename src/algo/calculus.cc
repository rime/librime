// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/algo/calculus.h>

namespace rime {

Calculus::Calculus() {
  Register(&Transliteration::Parse);
}

void Calculus::Register(Calculation::Factory* factory) {
  factories_.push_back(factory);
}

Calculation* Calculus::Parse(const std::string& definition) {
  Calculation* result = NULL;
  BOOST_FOREACH(Calculation::Factory* factory, factories_) {
    result = factory(definition);
    if (result)
      break;
  }
  return result;
}

bool Transliteration::Apply(const Spelling& input, Spelling* output) {
  // TODO:
  return false;
}

Calculation* Transliteration::Parse(const std::string& definition) {
  // TODO:
  return NULL;
}

}  // namespace rime
