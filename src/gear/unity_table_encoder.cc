//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-08-31 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/dict/dict_settings.h>
#include <rime/dict/user_dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/gear/unity_table_encoder.h>

namespace rime {

static const char* kEncodedPrefix = "\x7f""enc\x1f";

UnityTableEncoder::UnityTableEncoder(UserDictionary* user_dict)
    : TableEncoder(NULL), user_dict_(user_dict) {
  set_collector(this);
}

UnityTableEncoder::~UnityTableEncoder() {
}

bool UnityTableEncoder::Load(const Ticket& ticket) {
  auto c = ReverseLookupDictionary::Require("reverse_lookup_dictionary");
  if (!c) {
    LOG(ERROR) << "component not available: reverse_lookup_dictionary";
    return false;
  }
  rev_dict_.reset(c->Create(ticket));
  if (!rev_dict_ || !rev_dict_->Load()) {
    LOG(ERROR) << "error loading dictionary for unity table encoder.";
    return false;
  }
  auto settings = rev_dict_->GetDictSettings();
  if (!settings || !settings->use_rule_based_encoder()) {
    LOG(WARNING) << "unity table encoder is not enabled in dict settings.";
    return false;
  }
  return LoadSettings(settings.get());
}

void UnityTableEncoder::CreateEntry(const string& word,
                                    const string& code_str,
                                    const string& weight_str) {
  if (!user_dict_)
    return;
  DictEntry entry;
  entry.text = word;
  entry.custom_code = code_str + ' ';
  int commits = (weight_str == "0") ? 0 : 1;
  user_dict_->UpdateEntry(entry, commits, kEncodedPrefix);
}

bool UnityTableEncoder::TranslateWord(const string& word,
                                      vector<string>* code) {
  if (!rev_dict_) {
    return false;
  }
  string str_list;
  if (rev_dict_->LookupStems(word, &str_list) ||
      rev_dict_->ReverseLookup(word, &str_list)) {
    boost::split(*code, str_list, boost::is_any_of(" "));
    return !code->empty();
  }
  return false;
}

size_t UnityTableEncoder::LookupPhrases(UserDictEntryIterator* result,
                                        const string& input,
                                        bool predictive,
                                        size_t limit,
                                        string* resume_key) {
  if (!user_dict_)
    return 0;
  return user_dict_->LookupWords(result,
                                 kEncodedPrefix + input,
                                 predictive, limit, resume_key);
}

bool UnityTableEncoder::HasPrefix(const string& key) {
  return boost::starts_with(key, kEncodedPrefix);
}

bool UnityTableEncoder::AddPrefix(string* key) {
  key->insert(0, kEncodedPrefix);
  return true;
}

bool UnityTableEncoder::RemovePrefix(string* key) {
  if (!HasPrefix(*key))
    return false;
  key->erase(0, strlen(kEncodedPrefix));
  return true;
}

}  // namespace rime
