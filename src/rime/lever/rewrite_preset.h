//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_PRESET_H_
#define RIME_REWRITE_PRESET_H_

#include <memory>
#include <string>
#include <vector>

#include <rime/common.h>

namespace rime {

struct RewritePresetStage {
  string name;
  vector<path> files;
};

class RewritePresetCatalog {
 public:
  explicit RewritePresetCatalog(const path& root_path);
  ~RewritePresetCatalog();

  bool Load();
  bool Resolve(const string& preset_name,
               vector<RewritePresetStage>* stages) const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace rime

#endif  // RIME_REWRITE_PRESET_H_
