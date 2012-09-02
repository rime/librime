// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_R10N_TRANSLATOR_H_
#define RIME_R10N_TRANSLATOR_H_

#include <string>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/impl/translator_commons.h>

namespace rime {

class Code;
struct DictEntry;
struct DictEntryCollector;
class Dictionary;
class UserDictionary;
struct SyllableGraph;

class R10nTranslator : public Translator,
                       public Memory,
                       public TranslatorOptions {
 public:
  R10nTranslator(Engine *engine);
  virtual ~R10nTranslator();

  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);
  virtual bool Memorize(const DictEntry& commit_entry,
                        const std::vector<const DictEntry*>& elements);
  
  const std::string FormatPreedit(const std::string &preedit);
  const std::string Spell(const Code &code);

  // options
  int spelling_hints() const { return spelling_hints_; }
  
 protected:
  int spelling_hints_;
};

}  // namespace rime

#endif  // RIME_R10N_TRANSLATOR_H_
