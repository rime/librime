//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TABLE_TRANSLATOR_H_
#define RIME_TABLE_TRANSLATOR_H_

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

class Grammar;
class UnityTableEncoder;

class TableTranslator : public Translator,
                        public Memory,
                        public TranslatorOptions {
 public:
  TableTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                        const Segment& segment);
  virtual bool Memorize(const CommitEntry& commit_entry);

  an<Translation> MakeSentence(const string& input,
                                       size_t start,
                                       bool include_prefix_phrases = false);

  UnityTableEncoder* encoder() const { return encoder_.get(); }

 protected:
  bool enable_charset_filter_ = false;
  bool enable_encoder_ = false;
  bool enable_sentence_ = true;
  bool sentence_over_completion_ = false;
  bool encode_commit_history_ = true;
  int max_phrase_length_ = 5;
  int max_homographs_ = 1;
  the<UnityTableEncoder> encoder_;
  the<Grammar> grammar_;
};

class TableTranslation : public Translation {
 public:

  TableTranslation(TranslatorOptions* options,
                   const Language* language,
                   const string& input,
                   size_t start,
                   size_t end,
                   const string& preedit,
                   DictEntryIterator&& iter = {},
                   UserDictEntryIterator&& uter = {});

  virtual bool Next();
  virtual an<Candidate> Peek();

 protected:
  virtual bool FetchMoreUserPhrases() { return false; }
  virtual bool FetchMoreTableEntries() { return false; }

  bool CheckEmpty();
  bool PreferUserPhrase();

  an<DictEntry> PreferredEntry(bool prefer_user_phrase) {
    return prefer_user_phrase ? uter_.Peek() : iter_.Peek();
  }

  TranslatorOptions* options_;
  const Language* language_;
  string input_;
  size_t start_;
  size_t end_;
  string preedit_;
  DictEntryIterator iter_;
  UserDictEntryIterator uter_;
};

}  // namespace rime

#endif  // RIME_TABLE_TRANSLATOR_H_
