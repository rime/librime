//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-26 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CUSTOM_SETTINGS_H_
#define RIME_CUSTOM_SETTINGS_H_

#include <rime/config.h>

namespace rime {

class Deployer;

class CustomSettings {
 public:
  CustomSettings(Deployer* deployer,
                 const string& config_id,
                 const string& generator_id);
  virtual ~CustomSettings() = default;

  virtual bool Load();
  virtual bool Save();
  ConfigValuePtr GetValue(const string& key);
  ConfigListPtr GetList(const string& key);
  ConfigMapPtr GetMap(const string& key);
  bool Customize(const string& key, const ConfigItemPtr& item);
  bool IsFirstRun();
  bool modified() const { return modified_; }
  Config* config() { return &config_; }

 protected:
  Deployer* deployer_;
  bool modified_ = false;
  string config_id_;
  string generator_id_;
  Config config_;
  Config custom_config_;
};

}  // namespace rime

#endif  // RIME_CUSTOM_SETTINGS_H_
