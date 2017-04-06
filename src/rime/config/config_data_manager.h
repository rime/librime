//
// Copyright RIME Developers
// Distributed under the BSD License
//

#ifndef RIME_CONFIG_DATA_MANAGER_H_
#define RIME_CONFIG_DATA_MANAGER_H_

#include <rime/common.h>

namespace rime {

class ConfigData;

class ConfigDataManager : public map<string, weak<ConfigData>> {
 public:
  an<ConfigData> GetConfigData(const string& config_file_path);
  bool ReloadConfigData(const string& config_file_path);

  static ConfigDataManager& instance();

 private:
  ConfigDataManager() = default;
};

}  // namespace rime

#endif  // RIME_CONFIG_DATA_MANAGER_H_
