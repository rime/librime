//
// Copyright RIME Developers
// Distributed under the BSD License
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

  virtual an<Translation> Query(const string& input,
                                const Segment& segment);

 private:
  string Translate(const string& input);

  using TrivialDictionary = map<string, string>;
  TrivialDictionary dictionary_;
};

}  // namespace sample

#endif  // TRIVIAL_TRANSLATOR_H_
