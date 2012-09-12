// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-06-20 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ECHO_TRANSLATOR_H_
#define RIME_ECHO_TRANSLATOR_H_

#include <rime/translator.h>

namespace rime {

class EchoTranslator : public Translator {
 public:
  EchoTranslator(Engine *engine) : Translator(engine) {}
  virtual ~EchoTranslator() {}
  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);
};

}  // namespace rime

#endif  // RIME_ECHO_TRANSLATOR_H_
