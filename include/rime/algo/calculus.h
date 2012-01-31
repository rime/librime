// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_CALCULUS_H_
#define RIME_CALCULUS_H_

#include <map>
#include <string>
#include <vector>
#include <stdint.h>
#include "spelling.h"

namespace rime {

class Calculation {
 public:
  typedef Calculation* Factory(const std::string& definition);

  Calculation() {}
  virtual ~Calculation() {}
  
  virtual bool Apply(const Spelling& input, Spelling* output) = 0;
};

class Calculus {
 public:
  Calculus();
  void Register(Calculation::Factory* factory);
  Calculation::Factory Parse;

 private:
  std::vector<Calculation::Factory*> factories_;
};

class Transliteration : public Calculation {
 public:
  bool Apply(const Spelling& input, Spelling* output);
  static Factory Parse;
  
 protected:
  std::map<uint32_t, uint32_t> char_map_;
};

}  // namespace rime

#endif  // RIME_CALCULUS_H_
