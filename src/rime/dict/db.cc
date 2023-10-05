//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <rime/common.h>
#include <rime/resource.h>
#include <rime/service.h>
#include <rime/dict/db.h>

namespace rime {

// DbAccessor members

bool DbAccessor::MatchesPrefix(const string& key) {
  return boost::starts_with(key, prefix_);
}

// DbComponentBase

static const ResourceType kDbResourceType = {"db", "", ""};

DbComponentBase::DbComponentBase()
    : db_resource_resolver_(
          Service::instance().CreateResourceResolver(kDbResourceType)) {}

DbComponentBase::~DbComponentBase() {}

string DbComponentBase::DbFilePath(const string& name,
                                   const string& extension) const {
  return db_resource_resolver_->ResolvePath(name + extension).string();
}

// Db members

Db::Db(const string& file_name, const string& name)
    : name_(name), file_name_(file_name) {}

bool Db::Exists() const {
  return std::filesystem::exists(file_name());
}

bool Db::Remove() {
  if (loaded()) {
    LOG(ERROR) << "attempt to remove opened db '" << name_ << "'.";
    return false;
  }
  return std::filesystem::remove(file_name());
}

bool Db::CreateMetadata() {
  LOG(INFO) << "creating metadata for db '" << name_ << "'.";
  return MetaUpdate("/db_name", name_) &&
         MetaUpdate("/rime_version", RIME_VERSION);
}

}  // namespace rime
