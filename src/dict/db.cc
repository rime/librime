//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <rime/common.h>
#include <rime/service.h>
#include <rime/dict/db.h>

namespace rime {

// DbAccessor members

bool DbAccessor::MatchesPrefix(const string& key) {
  return boost::starts_with(key, prefix_);
}

// Db members

Db::Db(const string& name) : name_(name) {
  boost::filesystem::path path(name);
  if (path.has_parent_path()) {
    file_name_ = name;
  }
  else {
    boost::filesystem::path dir(Service::instance().deployer().user_data_dir);
    file_name_ = (dir / path).string();
  }
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
