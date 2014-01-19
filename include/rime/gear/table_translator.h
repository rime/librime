//
// Copyleft RIME Developers
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
#include <rime/dict/user_dictionary.h>
#include <rime/gear/memory.h>
#include <rime/gear/translator_commons.h>

namespace rime {

class UnityTableEncoder;

class TableTranslator : public Translator,
                        public Memory,
                        public TranslatorOptions {
 public:
  TableTranslator(const Ticket& ticket);

  virtual shared_ptr<Translation> Query(const std::string &input,
                                        const Segment &segment,
                                        std::string* prompt);
  virtual bool Memorize(const CommitEntry& commit_entry);

  shared_ptr<Translation> MakeSentence(const std::string& input,
                                       size_t start,
                                       bool include_prefix_phrases = false);

  UnityTableEncoder* encoder() const { return encoder_.get(); }

 protected:
  bool enable_charset_filter_;
  bool enable_encoder_;
  bool enable_sentence_;
  bool sentence_over_completion_;
  bool encode_commit_history_;
  int max_phrase_length_;
  scoped_ptr<UnityTableEncoder> encoder_;
};

class TableTranslation : public Translation {
 public:

  TableTranslation(TranslatorOptions* options, Language* language,
                   const std::string& input, size_t start, size_t end,
                   const std::string& preedit);
  TableTranslation(TranslatorOptions* options, Language* language,
                   const std::string& input, size_t start, size_t end,
                   const std::string& preedit,
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

  TranslatorOptions* options_;
  Language* language_;
  std::string input_;
  size_t start_;
  size_t end_;
  std::string preedit_;
  DictEntryIterator iter_;
  UserDictEntryIterator uter_;
};

}  // namespace rime

#endif  // RIME_TABLE_TRANSLATOR_H_
