//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_CUSTOMIZER_H_
#define RIME_CUSTOMIZER_H_

#include <rime/common.h>

namespace rime {

class Customizer {
 public:
  Customizer(const path& source_path,
             const path& dest_path,
             const string& version_key)
      : source_path_(source_path),
        dest_path_(dest_path),
        version_key_(version_key) {}

  // DEPRECATED: in favor of auto-patch config compiler plugin
  bool UpdateConfigFile();

 protected:
  path source_path_;
  path dest_path_;
  string version_key_;
};

}  // namespace rime

#endif  // RIME_CUSTOMIZER_H_
