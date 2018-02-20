//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-26 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/signature.h>
#include <rime/lever/custom_settings.h>

namespace fs = boost::filesystem;

namespace rime {

CustomSettings::CustomSettings(Deployer* deployer,
                               const string& config_id,
                               const string& generator_id)
    : deployer_(deployer),
      config_id_(config_id),
      generator_id_(generator_id) {
}

bool CustomSettings::Load() {
  fs::path user_data_path(deployer_->user_data_dir);
  fs::path shared_data_path(deployer_->shared_data_dir);
  fs::path config_path(user_data_path / "build" / (config_id_ + ".yaml"));
  if (!config_.LoadFromFile(config_path.string())) {
    config_path = shared_data_path / "build" / (config_id_ + ".yaml");
    if (!config_.LoadFromFile(config_path.string())) {
      LOG(WARNING) << "cannot find '" << config_id_ << ".yaml'.";
      return false;
    }
  }
  fs::path custom_config_path(user_data_path / (config_id_ + ".custom.yaml"));
  custom_config_.LoadFromFile(custom_config_path.string());
  modified_ = false;
  return true;
}

bool CustomSettings::Save() {
  if (!modified_)
    return false;
  Signature signature(generator_id_, "customization");
  signature.Sign(&custom_config_, deployer_);
  fs::path custom_config_path(deployer_->user_data_dir);
  custom_config_path /= config_id_ + ".custom.yaml";
  custom_config_.SaveToFile(custom_config_path.string());
  modified_ = false;
  return true;
}

an<ConfigValue> CustomSettings::GetValue(const string& key) {
  return config_.GetValue(key);
}

an<ConfigList> CustomSettings::GetList(const string& key) {
  return config_.GetList(key);
}

an<ConfigMap> CustomSettings::GetMap(const string& key) {
  return config_.GetMap(key);
}

bool CustomSettings::Customize(const string& key,
                               const an<ConfigItem>& item) {
  auto patch = custom_config_.GetMap("patch");
  if (!patch) {
    patch = New<ConfigMap>();
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
