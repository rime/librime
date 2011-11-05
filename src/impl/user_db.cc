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
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/impl/user_db.h>
#include <rime/impl/syllablizer.h>

namespace rime {

// UserDbAccessor memebers

bool UserDbAccessor::Yield(std::string *key, std::string *value) {
  if (!cursor_ || !key || !value)
    return false;
  return cursor_->get(key, value, true) && boost::starts_with(*key, key_);
}

bool UserDbAccessor::exhausted() {
  std::string key;
  return !cursor_->get_key(&key, false) || !boost::starts_with(key, key_);
}

// UserDb members

UserDb::UserDb(const std::string &name)
    : name_(name), loaded_(false) {
  boost::filesystem::path path(ConfigDataManager::instance().user_data_dir());
  file_name_ = (path / name).string() + ".userdb.kct";
  db_.reset(new kyotocabinet::TreeDB);
}

UserDb::~UserDb() {
  if (loaded())
    Close();
}

const UserDbAccessor UserDb::Query(const std::string &key) {
  if (!loaded())
    return UserDbAccessor();

  kyotocabinet::DB::Cursor *cursor = db_->cursor();
  cursor->jump(key);
  return UserDbAccessor(key, cursor);
}

bool UserDb::Fetch(const std::string &key, std::string *value) {
  if (!value || !loaded())
    return false;
  return db_->get(key, value);
}

bool UserDb::Update(const std::string &key, const std::string &value) {
  if (!loaded()) return false;
  return db_->set(key, value);
}

bool UserDb::Erase(const std::string &key) {
  if (!loaded()) return false;
  return db_->remove(key);
}

bool UserDb::Exists() const {
  return boost::filesystem::exists(file_name());
}

bool UserDb::Remove() {
  if (loaded()) {
    EZLOGGERPRINT("Error: attempt to remove open userdb '%s'.", name_.c_str());
    return false;
  }
  return boost::filesystem::remove(file_name());
}

bool UserDb::Open() {
  EZLOGGERFUNCTRACKER;
  if (loaded()) return false;
  loaded_ = db_->open(file_name());
  if (loaded_ && db_->count() == 0)
    CreateMetadata();
  return loaded_;
}

bool UserDb::Close() {
  if (!loaded()) return false;
  db_->close();
  loaded_ = false;
  return true;
}

bool UserDb::CreateMetadata() {
  // TODO: get version from installation info
  std::string rime_version("1.0");
  // TODO: get uuid from user profile
  std::string user_id("default_user");
  // '0x01' is the meta character
  return db_->set("\0x01/db_name", name_) &&
         db_->set("\0x01/rime_version", rime_version) &&
         db_->set("\0x01/user_id", user_id);
}

}  // namespace rime
