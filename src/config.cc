// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#include <fstream>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/config.h>

namespace rime {

struct ConfigData {
  ConfigItemPtr root;
  
  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);
  ConfigItemPtr Traverse(const std::string &key);

  static ConfigItemPtr ConvertFromYaml(const YAML::Node &yaml_node);
  static void EmitYaml(const ConfigItemPtr &node, YAML::Emitter *emitter);
};

// ConfigValue members

ConfigValue::ConfigValue(bool value)
    : ConfigItem(kScalar) {
  SetBool(value);
}

ConfigValue::ConfigValue(int value)
    : ConfigItem(kScalar) {
  SetInt(value);
}

ConfigValue::ConfigValue(double value)
    : ConfigItem(kScalar) {
  SetDouble(value);
}

ConfigValue::ConfigValue(const char *value)
    : ConfigItem(kScalar), value_(value) {
}

ConfigValue::ConfigValue(const std::string &value)
    : ConfigItem(kScalar), value_(value) {
}

bool ConfigValue::GetBool(bool *value) const {
  if (!value || value_.empty())
    return false;
  std::string bstr = value_;
  boost::to_lower(bstr);
  if ("true" == bstr) {
    *value = true;
    return true;
  }
  else if ("false" == bstr) {
    *value = false;
    return true;
  }
  else
    return false;
}

bool ConfigValue::GetInt(int *value) const {
  if (!value || value_.empty())
    return false;
  try {
    *value = boost::lexical_cast<int>(value_);
  }
  catch (...) {
    return false;
  }
  return true;
}

bool ConfigValue::GetDouble(double *value) const {
  if (!value || value_.empty())
    return false;
  try {
    *value = boost::lexical_cast<double>(value_);
  }
  catch (...) {
    return false;
  }
  return true;
}

bool ConfigValue::GetString(std::string *value) const {
  if (!value) return false;
  *value = value_;
  return true;
}

bool ConfigValue::SetBool(bool value) {
  value_ = value ? "true" : "false";
  return true;
}

bool ConfigValue::SetInt(int value) {
  value_ = boost::lexical_cast<std::string>(value);
  return true;
}

bool ConfigValue::SetDouble(double value) {
  value_ = boost::lexical_cast<std::string>(value);
  return true;
}

bool ConfigValue::SetString(const std::string &value) {
  value_ = value;
  return true;
}

// ConfigList members

ConfigItemPtr ConfigList::GetAt(size_t i) const {
  if (i >= seq_.size())
    return ConfigItemPtr();
  else
    return seq_[i];
}

ConfigValuePtr ConfigList::GetValueAt(size_t i) const {
  return As<ConfigValue>(GetAt(i));
}

bool ConfigList::SetAt(size_t i, const ConfigItemPtr &element) {
  if (i >= seq_.size())
    return false;
  seq_[i] = element;
  return true;
}

bool ConfigList::Append(const ConfigItemPtr &element) {
  seq_.push_back(element);
  return true;
}

bool ConfigList::Clear() {
  seq_.clear();
  return true;
}

size_t ConfigList::size() const {
  return seq_.size();
}

ConfigList::Iterator ConfigList::begin() {
  return seq_.begin();
}

ConfigList::Iterator ConfigList::end() {
  return seq_.end();
}

// ConfigMap members

bool ConfigMap::HasKey(const std::string &key) const {
  return bool(Get(key));
}

ConfigItemPtr ConfigMap::Get(const std::string &key) const {
  Map::const_iterator it = map_.find(key);
  if (it == map_.end())
    return ConfigItemPtr();
  else
    return it->second;
}

ConfigValuePtr ConfigMap::GetValue(const std::string &key) const {
  return As<ConfigValue>(Get(key));
}

bool ConfigMap::Set(const std::string &key, const ConfigItemPtr &element) {
  map_[key] = element;
  return true;
}

bool ConfigMap::Clear() {
  map_.clear();
  return true;
}

ConfigMap::Iterator ConfigMap::begin() {
  return map_.begin();
}

ConfigMap::Iterator ConfigMap::end() {
  return map_.end();
}

// Config members

Config::Config() : data_(new ConfigData) {
}

Config::~Config() {
}

Config::Config(const std::string &file_name) {
  data_ = ConfigDataManager::instance().GetConfigData(file_name);
}

bool Config::LoadFromFile(const std::string& file_name) {
  return data_->LoadFromFile(file_name);
}

bool Config::SaveToFile(const std::string& file_name) {
  return data_->SaveToFile(file_name);
}

bool Config::IsNull(const std::string &key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kNull;
}

bool Config::GetBool(const std::string& key, bool *value) {
  EZLOGGERVAR(key);
  ConfigValuePtr p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetBool(value);
}

bool Config::GetInt(const std::string& key, int *value) {
  EZLOGGERVAR(key);
  ConfigValuePtr p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetInt(value);
}

bool Config::GetDouble(const std::string& key, double *value) {
  EZLOGGERVAR(key);
  ConfigValuePtr p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetDouble(value);
}

bool Config::GetString(const std::string& key, std::string *value) {
  EZLOGGERVAR(key);
  ConfigValuePtr p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetString(value);
}

ConfigListPtr Config::GetList(const std::string& key) {
  EZLOGGERVAR(key);
  return As<ConfigList>(data_->Traverse(key));
}

ConfigMapPtr Config::GetMap(const std::string& key) {
  EZLOGGERVAR(key);
  return As<ConfigMap>(data_->Traverse(key));
}

bool Config::Set(const std::string &key, const ConfigItemPtr &item) {
  EZLOGGERVAR(key);
  if (key.empty() || key == "/") {
    data_->root = item;
    return true;
  }
  ConfigItemPtr p(data_->root);
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  size_t k = keys.size() - 1;
  for (size_t i = 0; i <= k; ++i) {
    if (!p || p->type() != ConfigItem::kMap)
      return false;
    if (i == k) {
      As<ConfigMap>(p)->Set(keys[i], item);
      return true;
    }
    else {
      ConfigItemPtr next(As<ConfigMap>(p)->Get(keys[i]));
      if (!next) {
        next = ConfigItemPtr(new ConfigMap);
        As<ConfigMap>(p)->Set(keys[i], next);
      }
      p = next;
    }
  }
  return false;
}
                 
// ConfigComponent members

const std::string ConfigComponent::GetConfigFilePath(const std::string &config_id) {
  return boost::str(boost::format(pattern_) % config_id);
}

Config* ConfigComponent::Create(const std::string &config_id) {
  const std::string path(GetConfigFilePath(config_id));
  EZLOGGERPRINT("config file path: %s", path.c_str());
  return new Config(path);
}

// ConfigDataManager memebers

scoped_ptr<ConfigDataManager> ConfigDataManager::instance_;

shared_ptr<ConfigData> ConfigDataManager::GetConfigData(const std::string &config_file_path) {
  shared_ptr<ConfigData> sp;
  // keep a weak reference to the shared config data in the manager
  weak_ptr<ConfigData> &wp((*this)[config_file_path]);
  if (wp.expired()) {  // create a new copy and load it
    sp.reset(new ConfigData);
    sp->LoadFromFile(config_file_path);
    wp = sp;
  }
  else {  // obtain the shared copy
    sp = wp.lock();
  }
  return sp;
}

bool ConfigDataManager::ReloadConfigData(const std::string &config_file_path) {
  iterator it = find(config_file_path);
  if (it == end()) {  // never loaded
    return false;
  }
  shared_ptr<ConfigData> sp = it->second.lock();
  if (!sp)  {  // already been freed
    erase(it);
    return false;
  }
  sp->LoadFromFile(config_file_path);
  return true;
}

// ConfigData members

bool ConfigData::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  YAML::Node doc;
  if (!parser.GetNextDocument(doc))
    return false;
  root = ConvertFromYaml(doc);
  return true;
}

bool ConfigData::SaveToFile(const std::string& file_name) {
  std::ofstream out(file_name.c_str());
  YAML::Emitter emitter;
  EmitYaml(root, &emitter);
  out << emitter.c_str();
  return true;
}

ConfigItemPtr ConfigData::Traverse(const std::string &key) {
  EZDBGONLYLOGGERPRINT("traverse: %s", key.c_str());
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  // find the YAML::Node, and wrap it!
  ConfigItemPtr p = root;
  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator end = keys.end();
  for (; it != end; ++it) {
    if (!p || p->type() != ConfigItem::kMap)
      return ConfigItemPtr();
    p = As<ConfigMap>(p)->Get(*it);
  }
  return p;
}

ConfigItemPtr ConfigData::ConvertFromYaml(const YAML::Node &node) {
  if (YAML::NodeType::Null == node.Type()) {
    return ConfigItemPtr();
  }
  if (YAML::NodeType::Scalar == node.Type()) {
    return ConfigItemPtr(new ConfigValue(node.to<std::string>()));
  }
  if (YAML::NodeType::Sequence == node.Type()) {
    ConfigListPtr config_list(new ConfigList);
    YAML::Iterator it = node.begin();
    YAML::Iterator end = node.end();
    for ( ; it != end; ++it) {
      config_list->Append(ConvertFromYaml(*it));
    }
    return config_list;
  }
  else if (YAML::NodeType::Map == node.Type()) {
    ConfigMapPtr config_map(new ConfigMap);
    YAML::Iterator it = node.begin();
    YAML::Iterator end = node.end();
    for ( ; it != end; ++it) {
      std::string key = it.first().to<std::string>();
      config_map->Set(key, ConvertFromYaml(it.second()));
    }
    return config_map;
  }
  return ConfigItemPtr();
}

void ConfigData::EmitYaml(const ConfigItemPtr &node, YAML::Emitter *emitter) {
  if (!node || !emitter) return;
  if (node->type() == ConfigItem::kScalar) {
    ConfigValuePtr config_value(As<ConfigValue>(node));
    *emitter << config_value->str();
  }
  else if (node->type() == ConfigItem::kList) {
    ConfigListPtr config_list(As<ConfigList>(node));
    ConfigList::Iterator it = config_list->begin();
    ConfigList::Iterator end = config_list->end();
    *emitter << YAML::BeginSeq;
    for ( ; it != end; ++it) {
      EmitYaml(*it, emitter);
    }
    *emitter << YAML::EndSeq;
  }
  else if (node->type() == ConfigItem::kMap) {
    ConfigMapPtr config_map(As<ConfigMap>(node));
    ConfigMap::Iterator it = config_map->begin();
    ConfigMap::Iterator end = config_map->end();
    *emitter << YAML::BeginMap;
    for ( ; it != end; ++it) {
      *emitter << YAML::Key << it->first;
      *emitter << YAML::Value;
      EmitYaml(it->second, emitter);
    }
    *emitter << YAML::EndMap;
  }
}

}  // namespace rime
