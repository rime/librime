//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <rime/resource_resolver.h>

namespace rime {

boost::filesystem::path ResourceResolver::ResolvePath(const string& resource_id) {
  return boost::filesystem::absolute(
      boost::filesystem::path(type_.prefix + resource_id + type_.suffix),
      root_path_);
}

boost::filesystem::path
FallbackResourceResolver::ResolvePath(const string& resource_id) {
  auto default_path = ResourceResolver::ResolvePath(resource_id);
  if (!boost::filesystem::exists(default_path)) {
    auto fallback_path = boost::filesystem::absolute(
        boost::filesystem::path(type_.prefix + resource_id + type_.suffix),
        fallback_root_path_);
    if (boost::filesystem::exists(fallback_path)) {
      return fallback_path;
    }
  }
  return default_path;
}

}  // namespace rime
