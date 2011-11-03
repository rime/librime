// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/impl/syllablizer.h>
#include <rime/impl/user_db.h>
#include <rime/impl/user_dictionary.h>

namespace rime {

// UserDictionary members

UserDictionary::UserDictionary(const shared_ptr<UserDb> &user_db)
    : db_(user_db), tick_(0) {
}

UserDictionary::~UserDictionary() {
}

void UserDictionary::Attach(const shared_ptr<Table> &table, const shared_ptr<Prism> &prism) {
  table_ = table;
  prism_ = prism;
}

bool UserDictionary::Load() {
  if (!db_ || !db_->Open())
    return false;
  if (!GetTickCount() && !Initialize())
    return false;
  return true;
}

bool UserDictionary::loaded() const {
  return db_ && db_->loaded();
}

shared_ptr<UserDictEntryCollector> UserDictionary::Lookup(const SyllableGraph &syllable_graph, int start_pos) {
  if (!table_ || !prism_ || !loaded())
    return shared_ptr<UserDictEntryCollector>();

  shared_ptr<UserDictEntryCollector> collector(new UserDictEntryCollector);
  // TODO:
  return collector;
}

bool UserDictionary::UpdateEntry(const DictEntry &entry, int commit) {
  std::string code_str(TranslateCodeToString(entry.code));
  std::string key(code_str + " " + entry.text);
  double weight = entry.weight;
  int commit_count = entry.commit_count;
  if (commit > 0) {
    // TODO:
    weight += commit;
    commit_count += commit;
  }
  else if (commit < 0) {
    commit_count = (std::min)(-1, -commit_count);
  }
  std::string value(boost::str(boost::format("c=%1% w=%2% t=%3%") % commit_count % weight % tick_));
  return db_->Update(key, value);
}

bool UserDictionary::UpdateTickCount(TickCount increment) {
  tick_ += increment;
  try {
    return db_->Update("\0x01/tick", boost::lexical_cast<std::string>(tick_));
  }
  catch (...) {
    return false;
  }
}

bool UserDictionary::Initialize() {
  // TODO:
  return db_->Update("\0x01/tick", "0");
}

bool UserDictionary::FetchTickCount() {
  std::string value;
  try {
    if (!db_->Fetch("\0x01/tick", &value))
      return false;
    tick_ = boost::lexical_cast<TickCount>(value);
    return true;
  }
  catch (...) {
    tick_ = 0;
    return false;
  }
}

const std::string UserDictionary::TranslateCodeToString(const Code &code) {
  // TODO:
  return "TODO";
}

// UserDictionaryComponent members

UserDictionaryComponent::UserDictionaryComponent() {
}

UserDictionary* UserDictionaryComponent::Create(Schema *schema) {
  if (!schema) return NULL;
  Config *config = schema->config();
  const std::string &schema_id(schema->schema_id());
  std::string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    EZLOGGERPRINT("Error: dictionary not specified in schema '%s'.",
                  schema_id.c_str());
    return NULL;
  }
  // obtain userdb object
  shared_ptr<UserDb> user_db(user_db_pool_[dict_name].lock());
  if (!user_db) {
    user_db.reset(new UserDb(dict_name));
    user_db_pool_[dict_name] = user_db;
  }
  return new UserDictionary(user_db);
}

}  // namespace rime
