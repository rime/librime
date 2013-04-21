//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <rime_version.h>
#include <rime/common.h>
#include <rime/service.h>
#include <rime/dict/db.h>

namespace rime {

// DbAccessor members

DbAccessor::DbAccessor() {
}

DbAccessor::DbAccessor(const std::string& prefix) : prefix_(prefix) {
}

DbAccessor::~DbAccessor() {
}

bool DbAccessor::MatchesPrefix(const std::string& key) {
  return boost::starts_with(key, prefix_);
}

// Db members

Db::Db(const std::string& name)
    : name_(name), loaded_(false), readonly_(false), disabled_(false) {
  boost::filesystem::path path(Service::instance().deployer().user_data_dir);
  file_name_ = (path / name).string();
}

Db::~Db() {
}

bool Db::Exists() const {
  return boost::filesystem::exists(file_name());
}

bool Db::Remove() {
  if (loaded()) {
    LOG(ERROR) << "attempt to remove opened db '" << name_ << "'.";
    return false;
  }
  return boost::filesystem::remove(file_name());
}

bool Db::CreateMetadata() {
  LOG(INFO) << "creating metadata for db '" << name_ << "'.";
  return MetaUpdate("/db_name", name_) &&
      MetaUpdate("/rime_version", RIME_VERSION);
}

}  // namespace rime
