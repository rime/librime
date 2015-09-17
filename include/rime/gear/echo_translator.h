//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-06-20 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ECHO_TRANSLATOR_H_
#define RIME_ECHO_TRANSLATOR_H_

#include <rime/translator.h>

namespace rime {

class EchoTranslator : public Translator {
 public:
  EchoTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                        const Segment& segment);
};

}  // namespace rime

#endif  // RIME_ECHO_TRANSLATOR_H_
