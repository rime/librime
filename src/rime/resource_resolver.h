//
// Copyright RIME Developers
// Distributed under the BSD License
//

#ifndef RIME_RESOURCE_RESOLVER_H_
#define RIME_RESOURCE_RESOLVER_H_

#include <boost/filesystem.hpp>
#include <rime/common.h>

namespace rime {

class ResourceResolver;

struct ResourceType {
  string name;
  string prefix;
  string suffix;
};

class ResourceResolver {
 public:
  explicit ResourceResolver(const ResourceType type) : type_(type) {
  }
  boost::filesystem::path ResolvePath(const string& resource_id);
  void set_root_path(const boost::filesystem::path& root_path) {
    root_path_ = root_path;
  }
  boost::filesystem::path root_path() const {
    return root_path_;
  }
 private:
  const ResourceType type_;
  boost::filesystem::path root_path_;
};

}  // namespace rime

#endif  // RIME_RESOURCE_RESOLVER_H_
