//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-03 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_TRANSLATOR_H_
#define RIME_REVERSE_LOOKUP_TRANSLATOR_H_

#include <string>
#include <rime/common.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>

namespace rime {

class Dictionary;
class ReverseLookupDictionary;
class TranslatorOptions;

class ReverseLookupTranslator : public Translator {
 public:
  ReverseLookupTranslator(const Ticket& ticket);

  virtual shared_ptr<Translation> Query(const std::string& input,
                                        const Segment& segment);

 protected:
  void Initialize();

  std::string tag_;
  bool initialized_ = false;
  unique_ptr<Dictionary> dict_;
  unique_ptr<ReverseLookupDictionary> rev_dict_;
  unique_ptr<TranslatorOptions> options_;
  std::string prefix_;
  std::string suffix_;
  std::string tips_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_TRANSLATOR_H_
