//
// Copyright RIME Developers
// Distributed under the BSD License
//

#ifndef RIME_RESOURCE_H_
#define RIME_RESOURCE_H_

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

class ResourceResolver;

struct ResourceType {
  string name;
  string prefix;
  string suffix;
};

class RIME_DLL ResourceResolver {
 public:
  explicit ResourceResolver(const ResourceType type) : type_(type) {}
  virtual ~ResourceResolver() {}
  virtual path ResolvePath(const string& resource_id);
  string ToResourceId(const string& file_path) const;
  string ToFilePath(const string& resource_id) const;
  void set_root_path(path root_path) { root_path_ = root_path; }
  path root_path() const { return root_path_; }

 protected:
  const ResourceType type_;
  path root_path_;
};

// try fallback path if target file doesn't exist in root path
class RIME_DLL FallbackResourceResolver : public ResourceResolver {
 public:
  explicit FallbackResourceResolver(const ResourceType& type)
      : ResourceResolver(type) {}
  path ResolvePath(const string& resource_id) override;
  void set_fallback_root_path(path fallback_root_path) {
    fallback_root_path_ = fallback_root_path;
  }

 private:
  path fallback_root_path_;
};

}  // namespace rime

#endif  // RIME_RESOURCE_H_
