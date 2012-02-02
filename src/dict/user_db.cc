// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <rime_version.h>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/dict/user_db.h>
#include <rime/algo/syllabifier.h>

namespace rime {

// TreeDbAccessor memebers

TreeDbAccessor::TreeDbAccessor(kyotocabinet::DB::Cursor *cursor,
                               const std::string &prefix)
    : cursor_(cursor), prefix_(prefix) {
  if (!prefix.empty())
    Forward(prefix);
}

TreeDbAccessor::~TreeDbAccessor() {
  if (cursor_) {
    delete cursor_;
    cursor_ = NULL;
  }
}

bool TreeDbAccessor::Forward(const std::string &key) {
  return cursor_ && cursor_->jump(key);
}

bool TreeDbAccessor::GetNextRecord(std::string *key, std::string *value) {
  if (!cursor_ || !key || !value)
    return false;
  return cursor_->get(key, value, true) && boost::starts_with(*key, prefix_);
}

bool TreeDbAccessor::exhausted() {
  std::string key;
  return !cursor_->get_key(&key, false) || !boost::starts_with(key, prefix_);
}

// TreeDb members

TreeDb::TreeDb(const std::string &name) : name_(name), loaded_(false) {
  boost::filesystem::path path(Service::instance().deployer().user_data_dir);
  file_name_ = (path / name).string();
  Initialize();
}

void TreeDb::Initialize() {
  db_.reset(new kyotocabinet::TreeDB);
  db_->tune_options(kyotocabinet::TreeDB::TLINEAR | kyotocabinet::TreeDB::TCOMPRESS);
  db_->tune_buckets(10LL * 1000);
  db_->tune_defrag(8);
  db_->tune_page(32768);
}

TreeDb::~TreeDb() {
  if (loaded())
    Close();
}

const TreeDbAccessor TreeDb::Query(const std::string &key) {
  if (!loaded())
    return TreeDbAccessor();
  kyotocabinet::DB::Cursor *cursor = db_->cursor();
  return TreeDbAccessor(cursor, key);
}

bool TreeDb::Fetch(const std::string &key, std::string *value) {
  if (!value || !loaded())
    return false;
  return db_->get(key, value);
}

bool TreeDb::Update(const std::string &key, const std::string &value) {
  if (!loaded()) return false;
  EZDBGONLYLOGGER(key, value);
  return db_->set(key, value);
}

bool TreeDb::Erase(const std::string &key) {
  if (!loaded()) return false;
  return db_->remove(key);
}

bool TreeDb::Exists() const {
  return boost::filesystem::exists(file_name());
}

bool TreeDb::Remove() {
  if (loaded()) {
    EZLOGGERPRINT("Error: attempt to remove opened db '%s'.", name_.c_str());
    return false;
  }
  return boost::filesystem::remove(file_name());
}

bool TreeDb::Open() {
  if (loaded()) return false;
  loaded_ = db_->open(file_name());
  if (loaded_) {
    std::string db_name;
    if (!Fetch("\x01/db_name", &db_name))
      CreateMetadata();
  }
  else {
    EZLOGGERPRINT("Error opening db '%s'.", name_.c_str());
  }
  return loaded_;
}

bool TreeDb::OpenReadOnly() {
  if (loaded()) return false;
  loaded_ = db_->open(file_name(), kyotocabinet::TreeDB::OREADER);
  if (!loaded_) {
    EZLOGGERPRINT("Error opening db '%s' read-only.", name_.c_str());
  }
  return loaded_;
}

bool TreeDb::Close() {
  if (!loaded()) return false;
  db_->close();
  loaded_ = false;
  return true;
}

bool TreeDb::CreateMetadata() {
  EZLOGGERPRINT("Creating metadata for db '%s'.", name_.c_str());
  std::string rime_version(RIME_VERSION);
  // '\x01' is the meta character
  return db_->set("\x01/db_name", name_) &&
         db_->set("\x01/rime_version", rime_version);
}

// UserDb members

UserDb::UserDb(const std::string &name) : TreeDb(name + ".userdb.kct") {
}

bool UserDb::CreateMetadata() {
  Deployer &deployer(Service::instance().deployer());
  std::string user_id(deployer.user_id);
  // '\x01' is the meta character
  return TreeDb::CreateMetadata() &&
      db_->set("\x01/db_type", "userdb") &&
      db_->set("\x01/user_id", user_id);
}

}  // namespace rime
