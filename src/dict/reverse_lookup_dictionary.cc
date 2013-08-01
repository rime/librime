//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
//
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/schema.h>
#include <rime/dict/dict_settings.h>
#include <rime/dict/reverse_lookup_dictionary.h>

namespace rime {

static const char* kStemKeySuffix = "\x1fstem";

ReverseDb::ReverseDb(const std::string& name)
    : TreeDb(name + ".reverse.bin", "reversedb") {
}

ReverseLookupDictionary::ReverseLookupDictionary(shared_ptr<ReverseDb> db)
    : db_(db) {
}

ReverseLookupDictionary::ReverseLookupDictionary(const std::string& dict_name)
    : db_(boost::make_shared<ReverseDb>(dict_name)) {
}

bool ReverseLookupDictionary::Load() {
  return db_ && db_->Exists() && db_->OpenReadOnly();
}

bool ReverseLookupDictionary::ReverseLookup(const std::string &text,
                                            std::string *result) {
  return db_ && db_->Fetch(text, result);
}

bool ReverseLookupDictionary::LookupStems(const std::string &text,
                                            std::string *result) {
return db_ && db_->Fetch(text + kStemKeySuffix, result);
}

bool ReverseLookupDictionary::Build(DictSettings* settings,
                                    const Syllabary& syllabary,
                                    const Vocabulary& vocabulary,
                                    const ReverseLookupTable& stems,
                                    uint32_t dict_file_checksum) {
  LOG(INFO) << "building reverse lookup dictionary...";
  if (db_->Exists())
    db_->Remove();
  if (!db_->Open())
    return false;
  ReverseLookupTable rev_table;
  int syllable_id = 0;
  BOOST_FOREACH(const std::string& syllable, syllabary) {
    Vocabulary::const_iterator it = vocabulary.find(syllable_id);
    if (it == vocabulary.end())
      continue;
    const DictEntryList& entries(it->second.entries);
    BOOST_FOREACH(const shared_ptr<DictEntry>& e, entries) {
      rev_table[e->text].insert(syllable);
    }
    ++syllable_id;
  }
  // save reverse lookup entries
  BOOST_FOREACH(const ReverseLookupTable::value_type& v, rev_table) {
    std::string code_list(boost::algorithm::join(v.second, " "));
    db_->Update(v.first, code_list);
  }
  // save stems
  BOOST_FOREACH(const ReverseLookupTable::value_type& v, stems) {
    std::string key(v.first + kStemKeySuffix);
    std::string code_list(boost::algorithm::join(v.second, " "));
    db_->Update(key, code_list);
  }
  // save metadata
  db_->MetaUpdate("/dict_file_checksum",
                 boost::lexical_cast<std::string>(dict_file_checksum));
  if (settings && settings->use_rule_based_encoder()) {
    std::ostringstream yaml;
    settings->SaveToStream(yaml);
    db_->MetaUpdate("/dict_settings", yaml.str());
  }
  db_->Close();
  return true;
}

uint32_t ReverseLookupDictionary::GetDictFileChecksum() {
  std::string checksum;
  if (db_->MetaFetch("/dict_file_checksum", &checksum)) {
    return boost::lexical_cast<uint32_t>(checksum);
  }
  return 0;
}

shared_ptr<DictSettings> ReverseLookupDictionary::GetDictSettings() {
  shared_ptr<DictSettings> settings;
  std::string yaml;
  if (db_->MetaFetch("/dict_settings", &yaml)) {
    std::istringstream iss(yaml);
    settings = make_shared<DictSettings>();
    if (!settings->LoadFromStream(iss)) {
      settings.reset();
    }
  }
  return settings;
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
