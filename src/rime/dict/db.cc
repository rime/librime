//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <rime/algo/strings.h>
#include <rime/common.h>
#include <rime/resource.h>
#include <rime/service.h>
#include <rime/dict/db.h>

namespace rime {

// DbAccessor members

bool DbAccessor::MatchesPrefix(string_view key) {
  return strings::starts_with(key, prefix_);
}

// DbComponentBase

static const ResourceType kDbResourceType = {"db", "", ""};

DbComponentBase::DbComponentBase()
    : db_resource_resolver_(
          Service::instance().CreateResourceResolver(kDbResourceType)) {}

DbComponentBase::~DbComponentBase() {}

path DbComponentBase::DbFilePath(string_view name,
                                 string_view extension) const {
  return db_resource_resolver_->ResolvePath(strings::concat(name, extension));
}

// Db members

Db::Db(const path& file_path, string_view name)
    : name_(name), file_path_(file_path) {}

bool Db::Exists() const {
  return std::filesystem::exists(file_path());
}

bool Db::Remove() {
  if (loaded()) {
    LOG(ERROR) << "attempt to remove opened db '" << name_ << "'.";
    return false;
  }
  return std::filesystem::remove(file_path());
}

bool Db::CreateMetadata() {
  LOG(INFO) << "creating metadata for db '" << name_ << "'.";
  return MetaUpdate("/db_name", name_) &&
         MetaUpdate("/rime_version", RIME_VERSION);
}

}  // namespace rime
