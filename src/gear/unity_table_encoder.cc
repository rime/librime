//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-08-31 GONG Chen <chen.sst@gmail.com>
//
#include <rime/dict/dict_settings.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/gear/unity_table_encoder.h>

namespace rime {

UnityTableEncoder::UnityTableEncoder(UserDictionary* user_dict)
    : TableEncoder(this), user_dict_(user_dict) {
}

UnityTableEncoder::~UnityTableEncoder() {
}

bool UnityTableEncoder::Load(const Ticket& ticket) {
  ReverseLookupDictionary::Component *c =
      ReverseLookupDictionary::Require("reverse_lookup_dictionary");
  if (c) {
    rev_dict_.reset(c->Create(ticket));
    if (rev_dict_ && rev_dict_->Load()) {
      shared_ptr<DictSettings> settings = rev_dict_->GetDictSettings();
      if (settings && settings->use_rule_based_encoder()) {
        LoadSettings(settings.get());
      }
    }
  }
  return loaded();
}

void UnityTableEncoder::CreateEntry(const std::string &word,
                                    const std::string &code_str,
                                    const std::string &weight_str) {
  // TODO:
}

bool UnityTableEncoder::TranslateWord(const std::string& word,
                                      std::vector<std::string>* code) {
  if (!rev_dict_) {
    return false;
  }
  // TODO:
  return false;
}

}  // namespace rime
