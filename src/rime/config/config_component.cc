//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-06 Zou Xu <zouivex@gmail.com>
//
#include <cstdlib>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <rime/config/config_component.h>
#include <rime/config/config_data.h>
#include <rime/config/config_data_manager.h>
#include <rime/config/config_types.h>

namespace rime {

Config::Config() : ConfigItemRef(New<ConfigData>()) {
}

Config::~Config() {
}

Config::Config(const string& file_name)
    : ConfigItemRef(ConfigDataManager::instance().GetConfigData(file_name)) {
}

bool Config::LoadFromStream(std::istream& stream) {
  return data_->LoadFromStream(stream);
}

bool Config::SaveToStream(std::ostream& stream) {
  return data_->SaveToStream(stream);
}

bool Config::LoadFromFile(const string& file_name) {
  return data_->LoadFromFile(file_name);
}

bool Config::SaveToFile(const string& file_name) {
  return data_->SaveToFile(file_name);
}

bool Config::IsNull(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kNull;
}

bool Config::IsValue(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kScalar;
}

bool Config::IsList(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kList;
}

bool Config::IsMap(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kMap;
}

bool Config::GetBool(const string& key, bool* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetBool(value);
}

bool Config::GetInt(const string& key, int* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetInt(value);
}

bool Config::GetDouble(const string& key, double* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetDouble(value);
}

bool Config::GetString(const string& key, string* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetString(value);
}

an<ConfigItem> Config::GetItem(const string& key) {
  DLOG(INFO) << "read: " << key;
  return data_->Traverse(key);
}

an<ConfigValue> Config::GetValue(const string& key) {
  DLOG(INFO) << "read: " << key;
  return As<ConfigValue>(data_->Traverse(key));
}

an<ConfigList> Config::GetList(const string& key) {
  DLOG(INFO) << "read: " << key;
  return As<ConfigList>(data_->Traverse(key));
}

an<ConfigMap> Config::GetMap(const string& key) {
  DLOG(INFO) << "read: " << key;
  return As<ConfigMap>(data_->Traverse(key));
}

bool Config::SetBool(const string& key, bool value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetInt(const string& key, int value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetDouble(const string& key, double value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetString(const string& key, const char* value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetString(const string& key, const string& value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetItem(const string& key, an<ConfigItem> item) {
  return data_->TraverseWrite(key, item);
}

an<ConfigItem> Config::GetItem() const {
  return data_->root;
}

void Config::SetItem(an<ConfigItem> item) {
  data_->root = item;
  set_modified();
}

string ConfigComponent::GetConfigFilePath(const string& config_id) {
  return boost::str(boost::format(pattern_) % config_id);
}

Config* ConfigComponent::Create(const string& config_id) {
  string path(GetConfigFilePath(config_id));
  DLOG(INFO) << "config file path: " << path;
  return new Config(path);
}

}  // namespace rime
