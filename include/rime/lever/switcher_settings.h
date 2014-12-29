//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SWITCHER_SETTINGS_H_
#define RIME_SWITCHER_SETTINGS_H_

#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "custom_settings.h"

namespace rime {

struct SchemaInfo {
  std::string schema_id;
  std::string name;
  std::string version;
  std::string author;
  std::string description;
  std::string file_path;
};

class SwitcherSettings : public CustomSettings {
 public:
  using SchemaList = std::vector<SchemaInfo>;
  // a list of schema_ids
  using Selection = std::vector<std::string>;
  
  explicit SwitcherSettings(Deployer* deployer);
  bool Load();
  bool Select(Selection selection);
  bool SetHotkeys(const std::string& hotkeys);
  
  const SchemaList& available() const { return available_; }
  const Selection& selection() const { return selection_; }
  const std::string& hotkeys() const { return hotkeys_; }

 private:
  void GetAvailableSchemasFromDirectory(const boost::filesystem::path& dir);
  void GetSelectedSchemasFromConfig();
  void GetHotkeysFromConfig();

  SchemaList available_;
  Selection selection_;
  std::string hotkeys_;
};

}  // namespace rime

#endif  // RIME_SWITCHER_SETTINGS_H_
