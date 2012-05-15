// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-02-26 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/expl/signature.h>
#include <rime/expl/custom_settings.h>

namespace fs = boost::filesystem;

namespace rime {

CustomSettings::CustomSettings(Deployer* deployer,
                               const std::string& config_id,
                               const std::string& generator_id)
    : deployer_(deployer), modified_(false),
      config_id_(config_id), generator_id_(generator_id) {
}

bool CustomSettings::Load() {
  fs::path user_data_path(deployer_->user_data_dir);
  fs::path shared_data_path(deployer_->shared_data_dir);
  fs::path config_path(user_data_path / (config_id_ + ".yaml"));
  if (!config_.LoadFromFile(config_path.string())) {
    config_path = shared_data_path / (config_id_ + ".yaml");
    if (!config_.LoadFromFile(config_path.string())) {
      EZLOGGERPRINT("Warning: cannot find '%s.yaml'.", config_id_.c_str());
      return false;
    }
  }
  fs::path custom_config_path(user_data_path / (config_id_ + ".custom.yaml"));
  custom_config_.LoadFromFile(custom_config_path.string());
  modified_ = false;
  return true;
}

bool CustomSettings::Save() {
  if (!modified_) return false;
  Signature signature(generator_id_);
  signature.Sign(&custom_config_, deployer_);
  fs::path custom_config_path(deployer_->user_data_dir);
  custom_config_path /= config_id_ + ".custom.yaml";
  custom_config_.SaveToFile(custom_config_path.string());
  modified_ = false;
  return true;
}

ConfigValuePtr CustomSettings::GetValue(const std::string& key) {
  return config_.GetValue(key);
}

ConfigListPtr CustomSettings::GetList(const std::string& key) {
  return config_.GetList(key);
}

ConfigMapPtr CustomSettings::GetMap(const std::string& key) {
  return config_.GetMap(key);
}

bool CustomSettings::Customize(const std::string& key,
                               const ConfigItemPtr& item) {
  ConfigMapPtr patch = custom_config_.GetMap("patch");
  if (!patch) {
    patch = make_shared<ConfigMap>();
  }
  patch->Set(key, item);
  // the branch 'patch' should be set as a whole in order to be saved, for
  // its sub-key may contain slashes which disables directly setting a sub-item
  custom_config_.SetItem("patch", patch);
  modified_ = true;
  return true;
}

bool CustomSettings::IsFirstRun() {
  fs::path custom_config_path(deployer_->user_data_dir);
  custom_config_path /= config_id_ + ".custom.yaml";
  Config config;
  if (!config.LoadFromFile(custom_config_path.string()))
    return true;
  return !config.GetMap("customization");
}

}  // namespace rime
