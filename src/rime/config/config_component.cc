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
  return data_->LoadFromFile(file_name, nullptr);
}

bool Config::SaveToFile(const string& file_name) {
  return data_->SaveToFile(file_name);
}

bool Config::IsNull(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kNull;
}

bool Config::IsValue(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kScalar;
}

bool Config::IsList(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kList;
}

bool Config::IsMap(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kMap;
}

bool Config::GetBool(const string& path, bool* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetBool(value);
}

bool Config::GetInt(const string& path, int* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetInt(value);
}

bool Config::GetDouble(const string& path, double* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetDouble(value);
}

bool Config::GetString(const string& path, string* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetString(value);
}

size_t Config::GetListSize(const string& path) {
  DLOG(INFO) << "read: " << path;
  auto list = GetList(path);
  return list ? list->size() : 0;
}

an<ConfigItem> Config::GetItem(const string& path) {
  DLOG(INFO) << "read: " << path;
  return data_->Traverse(path);
}

an<ConfigValue> Config::GetValue(const string& path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigValue>(data_->Traverse(path));
}

an<ConfigList> Config::GetList(const string& path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigList>(data_->Traverse(path));
}

an<ConfigMap> Config::GetMap(const string& path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigMap>(data_->Traverse(path));
}

bool Config::SetBool(const string& path, bool value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetInt(const string& path, int value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetDouble(const string& path, double value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(const string& path, const char* value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(const string& path, const string& value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetItem(const string& path, an<ConfigItem> item) {
  return data_->TraverseWrite(path, item);
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
  string file_path(GetConfigFilePath(config_id));
  DLOG(INFO) << "config file path: " << file_path;
  return new Config(file_path);
}

}  // namespace rime
