// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-04-26 Wensong He <snowhws@gmail.com>
//
#ifndef RIME_TRIVIAL_TRANSLATOR_H_
#define RIME_TRIVIAL_TRANSLATOR_H_

#include <rime/translator.h>

namespace rime {
  
class TrivialTranslator : public Translator {
 public:
  TrivialTranslator(Engine *engine) : Translator(engine) {}
  virtual ~TrivialTranslator() {}
  virtual Translation* Query(const std::string &input,
                             const Segment &segment);
 private:
  
};

}  // namespace rime

#endif  // RIME_TRIVIAL_TRANSLATOR_H_


