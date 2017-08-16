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

}  // namespace rime
