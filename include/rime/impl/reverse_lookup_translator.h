// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-01-03 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_TRANSLATOR_H_
#define RIME_REVERSE_LOOKUP_TRANSLATOR_H_

#include <string>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/algo/algebra.h>

namespace rime {

class Dictionary;
class ReverseLookupDictionary;

class ReverseLookupTranslator : public Translator {
 public:
  ReverseLookupTranslator(Engine *engine);

  virtual Translation* Query(const std::string &input,
                             const Segment &segment);
 protected:
  scoped_ptr<Dictionary> dict_;
  scoped_ptr<ReverseLookupDictionary> rev_dict_;
  std::string prefix_;
  std::string tips_;
  Projection preedit_formatter_;
  Projection comment_formatter_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_TRANSLATOR_H_
