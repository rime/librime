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

#include <set>
#include <string>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/impl/translator_commons.h>

namespace rime {

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
                   const DictEntryIterator& iter,
                   const UserDictEntryIterator& uter = UserDictEntryIterator());
  
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();
  
 protected:
  virtual bool FetchMoreUserPhrases() { return false; }
  virtual bool FetchMoreTableEntries() { return false; }
  
  bool CheckEmpty();
  bool PreferUserPhrase();
  
  shared_ptr<DictEntry> PreferedEntry(bool prefer_user_phrase) {
    return prefer_user_phrase ? uter_.Peek() : iter_.Peek();
  }

  bool HasCandidate(const std::string& text) const {
    return candidate_set_.find(text) != candidate_set_.end();
  }
  
  TranslatorOptions* options_;
  Language* language_;
  std::string input_;
  size_t start_;
  size_t end_;
  std::string preedit_;
  DictEntryIterator iter_;
  UserDictEntryIterator uter_;
  std::set<std::string> candidate_set_;
};

}  // namespace rime

#endif  // RIME_TABLE_TRANSLATOR_H_
