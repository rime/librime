//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
//
#include <rime/schema.h>
#include <rime/dict/reverse_lookup_dictionary.h>

namespace rime {

ReverseLookupDictionary::ReverseLookupDictionary(shared_ptr<ReverseDb> db)
    : db_(db) {
}

bool ReverseLookupDictionary::Load() {
  return db_ && db_->Exists() && db_->OpenReadOnly();
}

bool ReverseLookupDictionary::ReverseLookup(const std::string &text,
                                            std::string *result) {
  return db_ && db_->Fetch(text, result);
}

ReverseLookupDictionaryComponent::ReverseLookupDictionaryComponent() {
}

ReverseLookupDictionary* ReverseLookupDictionaryComponent::Create(
    const Ticket& ticket) {
  if (!ticket.schema) return NULL;
  Config *config = ticket.schema->config();
  std::string reverse_lookup;
  if (!config->GetString(ticket.name_space + "/dictionary", &reverse_lookup)) {
    // reverse lookup not enabled
    return NULL;
  }
  std::string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    // missing!
    return NULL;
  }
  shared_ptr<ReverseDb> db(db_pool_[dict_name].lock());
  if (!db) {
    db = boost::make_shared<ReverseDb>(dict_name);
    db_pool_[dict_name] = db;
  }
  return new ReverseLookupDictionary(db);
}

}  // namespace rime
