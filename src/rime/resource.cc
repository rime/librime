//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <boost/algorithm/string.hpp>
#include <rime/fs.h>
#include <rime/resource.h>

namespace rime {

string ResourceResolver::ToResourceId(const string& file_path) const {
  string path_string = fs::path(file_path).generic_string();
  bool has_prefix = boost::starts_with(path_string, type_.prefix);
  bool has_suffix = boost::ends_with(path_string, type_.suffix);
  size_t start = (has_prefix ? type_.prefix.length() : 0);
  size_t end = path_string.length() - (has_suffix ? type_.suffix.length() : 0);
  return path_string.substr(start, end);
}

string ResourceResolver::ToFilePath(const string& resource_id) const {
  fs::path file_path(resource_id);
  bool missing_prefix = !file_path.has_parent_path() &&
                        !boost::starts_with(resource_id, type_.prefix);
  bool missing_suffix = !boost::ends_with(resource_id, type_.suffix);
  return (missing_prefix ? type_.prefix : "") + resource_id +
         (missing_suffix ? type_.suffix : "");
}

fs::path ResourceResolver::ResolvePath(const string& resource_id) {
  return fs::absolute(fs::path(type_.prefix + resource_id + type_.suffix),
                      root_path_);
}

fs::path FallbackResourceResolver::ResolvePath(const string& resource_id) {
  auto default_path = ResourceResolver::ResolvePath(resource_id);
  if (!fs::exists(default_path)) {
    auto fallback_path =
        fs::absolute(fs::path(type_.prefix + resource_id + type_.suffix),
                     fallback_root_path_);
    if (fs::exists(fallback_path)) {
      return fallback_path;
    }
  }
  return default_path;
}

}  // namespace rime
