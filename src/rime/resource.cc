//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <rime/resource.h>

namespace rime {

string ResourceResolver::ToResourceId(const string& file_path) const {
  // 将路径转换为 std::filesystem::path 对象来正确处理路径
  path p(file_path);
  string filename = p.filename().generic_u8string();
  
  bool has_prefix = boost::starts_with(filename, type_.prefix);
  bool has_suffix = boost::ends_with(filename, type_.suffix);
  size_t start = (has_prefix ? type_.prefix.length() : 0);
  size_t end = filename.length() - (has_suffix ? type_.suffix.length() : 0);
  
  // 如果原始路径包含目录部分，保留目录结构
  if (p.has_parent_path()) {
    string parent_dir = p.parent_path().generic_u8string();
    string resource_name = filename.substr(start, end);
    
    // 确保父目录路径不以 / 开头，使其成为相对路径
    if (!parent_dir.empty() && parent_dir[0] == '/') {
      parent_dir = parent_dir.substr(1); // 移除开头的 /
    }
    
    if (parent_dir.empty()) {
      return resource_name;
    } else {
      return parent_dir + "/" + resource_name;
    }
  } else {
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
