//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_CUSTOMIZER_H_
#define RIME_CUSTOMIZER_H_

namespace rime {

class Customizer {
 public:
  Customizer(const fs::path& source_path,
             const fs::path& dest_path,
             const string& version_key)
      : source_path_(source_path),
        dest_path_(dest_path),
        version_key_(version_key) {}

  // DEPRECATED: in favor of auto-patch config compiler plugin
  bool UpdateConfigFile();

 protected:
  fs::path source_path_;
  fs::path dest_path_;
  string version_key_;
};

}  // namespace rime

#endif  // RIME_CUSTOMIZER_H_
