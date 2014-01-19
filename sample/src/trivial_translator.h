//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-04-26 Wensong He <snowhws@gmail.com>
// 2013-10-20 GONG Chen <chen.sst@gmail.com>
//
#ifndef TRIVIAL_TRANSLATOR_H_
#define TRIVIAL_TRANSLATOR_H_

#include <map>
#include <string>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace sample {

using namespace rime;

class TrivialTranslator : public Translator {
 public:
  TrivialTranslator(const Ticket& ticket);

  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);

 private:
  std::string Translate(const std::string &input);

  typedef std::map<std::string, std::string> TrivialDictionary;
  TrivialDictionary dictionary_;
};

}  // namespace sample

#endif  // TRIVIAL_TRANSLATOR_H_
