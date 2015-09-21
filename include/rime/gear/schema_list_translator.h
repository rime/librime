//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-05-26 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SCHEMA_LIST_TRANSLATOR_H_
#define RIME_SCHEMA_LIST_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translator.h>

namespace rime {

class SchemaListTranslator : public Translator {
 public:
  SchemaListTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                        const Segment& segment);
};

}  // namespace rime

#endif  // RIME_SCHEMA_LIST_TRANSLATOR_H_
