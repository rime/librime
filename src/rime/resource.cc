//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <rime/resource.h>

namespace rime {

// Convert a file path to a resource ID by extracting the core name and
// directory structure
string ResourceResolver::ToResourceId(const string& file_path) const {
  path p(file_path);
  // Get the filename from the path
  string filename = p.filename().generic_u8string();

  // Check if the filename has the expected prefix and suffix
  bool has_prefix = boost::starts_with(filename, type_.prefix);
  bool has_suffix = boost::ends_with(filename, type_.suffix);

  // Calculate the start and end positions to extract the core name
  size_t start = (has_prefix ? type_.prefix.length() : 0);
  size_t end = filename.length() - (has_suffix ? type_.suffix.length() : 0);

  // Handle files with parent directories
  if (p.has_parent_path()) {
    string parent_dir = p.parent_path().generic_u8string();
    string resource_name = filename.substr(start, end);

    // Remove leading slash from parent directory if present
    if (!parent_dir.empty() && parent_dir[0] == '/') {
      parent_dir = parent_dir.substr(1);
    }

    // Return either just the resource name or include the parent directory
    if (parent_dir.empty()) {
      return resource_name;
    } else {
      return parent_dir + "/" + resource_name;
    }
  } else {
    // For files without parent paths, just return the extracted name
    return filename.substr(start, end);
  }
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
