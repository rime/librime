// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TABLE_TRANSLATOR_H_
#define RIME_TABLE_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

class Dictionary;

class TableTranslator : public Translator {
 public:
  TableTranslator(Engine *engine);
  virtual ~TableTranslator();

  virtual Translation* Query(const std::string &input,
                             const Segment &segment);

 private:
  scoped_ptr<Dictionary> dict_;
};

}  // namespace rime

#endif  // RIME_TABLE_TRANSLATOR_H_
