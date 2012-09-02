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

#include <string>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/dict/dictionary.h>
#include <rime/impl/translator_commons.h>

namespace rime {

class Dictionary;
class UserDictionary;

class TableTranslator : public Translator,
                        public Memory,
                        public TranslatorOptions {
 public:
  TableTranslator(Engine *engine);
  virtual ~TableTranslator();

  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);
  virtual bool Memorize(const DictEntry& commit_entry,
                        const std::vector<const DictEntry*>& elements);
  

  shared_ptr<Translation> MakeSentence(const std::string &input,
                                       size_t start);

 protected:
  bool enable_charset_filter_;
};

class TableTranslation : public Translation {
 public:
  TableTranslation(TranslatorOptions* options, Language* language,
                   const std::string& input, size_t start, size_t end);
  TableTranslation(TranslatorOptions* options, Language* language,
                   const std::string& input, size_t start, size_t end,
                   const DictEntryIterator& iter);
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();
  
 protected:
  DictEntryIterator iter_;
  TranslatorOptions* options_;
  Language* language_;
  std::string input_;
  size_t start_;
  size_t end_;
  std::string preedit_;
};

}  // namespace rime

#endif  // RIME_TABLE_TRANSLATOR_H_
