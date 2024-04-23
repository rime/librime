//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <rime/resource.h>

namespace rime {

string ResourceResolver::ToResourceId(const string& file_path) const {
  string string_path = path(file_path).generic_u8string();
  bool has_prefix = boost::starts_with(string_path, type_.prefix);
  bool has_suffix = boost::ends_with(string_path, type_.suffix);
  size_t start = (has_prefix ? type_.prefix.length() : 0);
  size_t end = string_path.length() - (has_suffix ? type_.suffix.length() : 0);
  return string_path.substr(start, end);
}

string ResourceResolver::ToFilePath(const string& resource_id) const {
  bool missing_prefix = !path(resource_id).has_parent_path() &&
                        !boost::starts_with(resource_id, type_.prefix);
  bool missing_suffix = !boost::ends_with(resource_id, type_.suffix);
  return (missing_prefix ? type_.prefix : "") + resource_id +
         (missing_suffix ? type_.suffix : "");
}

path ResourceResolver::ResolvePath(const string& resource_id) {
  return std::filesystem::absolute(root_path_ /
                                   (type_.prefix + resource_id + type_.suffix));
}

path FallbackResourceResolver::ResolvePath(const string& resource_id) {
  auto default_path = ResourceResolver::ResolvePath(resource_id);
  if (!std::filesystem::exists(default_path)) {
    auto fallback_path = std::filesystem::absolute(
        fallback_root_path_ / (type_.prefix + resource_id + type_.suffix));
    if (std::filesystem::exists(fallback_path)) {
      return fallback_path;
    }
  }
  return default_path;
}

}  // namespace rime
