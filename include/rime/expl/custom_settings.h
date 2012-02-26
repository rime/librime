// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-02-26 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CUSTOM_SETTINGS_H_
#define RIME_CUSTOM_SETTINGS_H_

#include <string>
#include <rime/config.h>

namespace rime {

class Deployer;

class CustomSettings {
 public:
  CustomSettings(Deployer* deployer,
                 const std::string& config_id,
                 const std::string& generator_id);
  virtual ~CustomSettings() {}
  virtual bool Load();
  virtual bool Save();
  ConfigValuePtr GetValue(const std::string& key);
  ConfigListPtr GetList(const std::string& key);
  ConfigMapPtr GetMap(const std::string& key);
  bool Customize(const std::string& key, const ConfigItemPtr& item);
  bool IsFirstRun();
  bool modified() const { return modified_; }

 protected:
  Deployer* deployer_;
  bool modified_;
  std::string config_id_;
  std::string generator_id_;
  Config config_;
  Config custom_config_;
};

}  // namespace rime

#endif  // RIME_CUSTOM_SETTINGS_H_
