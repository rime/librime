//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-08-31 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UNITY_TABLE_ENCODER_H_
#define RIME_UNITY_TABLE_ENCODER_H_

#include <rime/common.h>
#include <rime/algo/encoder.h>

namespace rime {

struct Ticket;
class ReverseLookupDictionary;
class UserDictionary;

class UnityTableEncoder : public TableEncoder, public PhraseCollector {
 public:
  UnityTableEncoder(UserDictionary* user_dict);
  ~UnityTableEncoder();

  bool Load(const Ticket& ticket);

  void CreateEntry(const std::string& word,
                   const std::string& code_str,
                   const std::string& weight_str);
  bool TranslateWord(const std::string& word,
                     std::vector<std::string>* code);

  size_t LookupPhrases(UserDictEntryIterator* result,
                       const std::string& input,
                       bool predictive,
                       size_t limit = 0,
                       std::string* resume_key = NULL);

  static bool HasPrefix(const std::string& key);
  static bool AddPrefix(std::string* key);
  static bool RemovePrefix(std::string* key);

 protected:
  UserDictionary* user_dict_;
  unique_ptr<ReverseLookupDictionary> rev_dict_;
};

}  // namespace rime

#endif  // RIME_UNITY_TABLE_ENCODER_H_
