//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-26 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/signature.h>
#include <rime/lever/custom_settings.h>

namespace rime {

static string remove_suffix(const string& input, const string& suffix) {
  return boost::ends_with(input, suffix)
             ? input.substr(0, input.length() - suffix.length())
             : input;
}

static string custom_config_file(const string& config_id) {
  return remove_suffix(config_id, ".schema") + ".custom.yaml";
}

CustomSettings::CustomSettings(Deployer* deployer,
                               const string& config_id,
                               const string& generator_id)
    : deployer_(deployer), config_id_(config_id), generator_id_(generator_id) {}

bool CustomSettings::Load() {
  path config_path = deployer_->staging_dir / (config_id_ + ".yaml");
  if (!config_.LoadFromFile(config_path)) {
    config_path = deployer_->prebuilt_data_dir / (config_id_ + ".yaml");
    if (!config_.LoadFromFile(config_path)) {
      LOG(WARNING) << "cannot find '" << config_id_ << ".yaml'.";
    }
  }
  path custom_config_path =
      deployer_->user_data_dir / custom_config_file(config_id_);
  if (!custom_config_.LoadFromFile(custom_config_path)) {
    return false;
  }
  modified_ = false;
  return true;
}

bool CustomSettings::Save() {
  if (!modified_)
    return false;
  Signature signature(generator_id_, "customization");
  signature.Sign(&custom_config_, deployer_);
  path custom_config_path(deployer_->user_data_dir);
  custom_config_path /= custom_config_file(config_id_);
  custom_config_.SaveToFile(custom_config_path);
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

bool CustomSettings::Customize(const string& key, const an<ConfigItem>& item) {
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
  path custom_config_path =
      deployer_->user_data_dir / custom_config_file(config_id_);
  Config config;
  if (!config.LoadFromFile(custom_config_path))
    return true;
  return !config.GetMap("customization");
}

}  // namespace rime
