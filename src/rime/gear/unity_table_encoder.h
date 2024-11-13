//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-08-31 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UNITY_TABLE_ENCODER_H_
#define RIME_UNITY_TABLE_ENCODER_H_

#include <rime/common.h>
#include <rime/algo/encoder.h>
#include <rime/dict/user_dictionary.h>

namespace rime {

struct Ticket;
class ReverseLookupDictionary;
class UserDictionary;

class UnityTableEncoder : public TableEncoder, public PhraseCollector {
 public:
  UnityTableEncoder(UserDictionary* user_dict);
  ~UnityTableEncoder();

  bool Load(const Ticket& ticket);

  void CreateEntry(string_view word,
                   string_view code_str,
                   string_view weight_str);
  bool TranslateWord(string_view word, vector<string>* code);

  size_t LookupPhrases(UserDictEntryIterator* result,
                       string_view input,
                       bool predictive,
                       size_t limit = 0,
                       string* resume_key = NULL);

  static bool HasPrefix(string_view key);
  static bool AddPrefix(string* key);
  static bool RemovePrefix(string* key);

 protected:
  UserDictionary* user_dict_;
  the<ReverseLookupDictionary> rev_dict_;
};

}  // namespace rime

#endif  // RIME_UNITY_TABLE_ENCODER_H_
