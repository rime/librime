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

#include <map>
#include <string>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

class TrivialTranslator : public Translator {
 public:
  TrivialTranslator(Engine *engine);
  virtual ~TrivialTranslator() {}

  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);

 private:
  const std::string Translate(const std::string &input);

  typedef std::map<std::string, std::string> TrivialDictionary;
  TrivialDictionary dictionary_;
};

}  // namespace rime

#endif  // RIME_TRIVIAL_TRANSLATOR_H_
