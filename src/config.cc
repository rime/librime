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

// private classes

struct ConfigItemData {
  typedef std::vector<ConfigItemPtr> Sequence;
  typedef std::map<std::string, ConfigItemPtr> Map;
  Sequence seq_children;
  Map map_children;
  std::string value;
};

class ConfigData {
 public:
  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);
  ConfigItemPtr Traverse(const std::string &key);
  static ConfigItemPtr ConvertFromYaml(const YAML::Node &yaml_node);
  static void EmitYaml(const ConfigItemPtr &node, YAML::Emitter *emitter);
 private:
  ConfigItemPtr root_;
};

// ConfigItem members

ConfigItem::ConfigItem()
    : type_(kNull), data_(new ConfigItemData) {
}

ConfigItem::ConfigItem(ValueType type)
    : type_(type), data_(new ConfigItemData) {
}

ConfigItem::ConfigItem(ValueType type, ConfigItemData *data)
    : type_(type), data_(data) {
}

ConfigItem::~ConfigItem() {
}

bool ConfigItem::GetBool(bool *value) const {
  if(type_ != ConfigItem::kScalar ||
     !data_ || data_->value.empty())
    return false;
  std::string bstr = data_->value;
  boost::to_lower(bstr);
  if("true" == bstr || "yes" == bstr || "y" == bstr) {
    *value = true;
    return true;
  }
  else if("false" == bstr || "no" == bstr || "n" == bstr) {
    *value = false;
    return true;
  }
  else
    return false;
}

bool ConfigItem::GetInt(int *value) const {
  if(type_ != ConfigItem::kScalar ||
     !data_ || data_->value.empty())
    return false;
  *value = boost::lexical_cast<int>(data_->value);
  return true;
}

bool ConfigItem::GetDouble(double *value) const {
  if(type_ != ConfigItem::kScalar ||
     !data_ || data_->value.empty())
    return false;
  *value = boost::lexical_cast<double>(data_->value);
  return true;
}

bool ConfigItem::GetString(std::string *value) const {
  if(type_ != ConfigItem::kScalar || !data_)
    return false;
  *value = data_->value;
  return true;
}

bool ConfigItem::SetBool(bool value) {
  if(type_ > ConfigItem::kScalar || !data_)
    return false;
  data_->value = value ? "true" : "false";
  return true;
}

bool ConfigItem::SetInt(int value) {
  if(type_ > ConfigItem::kScalar || !data_)
    return false;
  data_->value = boost::lexical_cast<std::string>(value);
  return true;
}

bool ConfigItem::SetDouble(double value) {
  if(type_ > ConfigItem::kScalar || !data_)
    return false;
  data_->value = boost::lexical_cast<std::string>(value);
  return true;
}

bool ConfigItem::SetString(const std::string &value) {
  if(type_ > ConfigItem::kScalar || !data_)
    return false;
  data_->value = value;
  return true;
}

// ConfigList members

ConfigItemPtr ConfigList::GetAt(size_t i) const {
  if (type_ != ConfigItem::kList
      || !data_ || i >= data_->seq_children.size())
    return ConfigItemPtr();
  else
    return data_->seq_children[i];
}

bool ConfigList::SetAt(size_t i, const ConfigItemPtr element) {
  if (type_ != ConfigItem::kList ||
      !data_ || i >= data_->seq_children.size())
    return false;
  data_->seq_children[i] = element;
  return true;
}

bool ConfigList::Append(const ConfigItemPtr element) {
  if (type_ != ConfigItem::kList || !data_)
    return false;
  data_->seq_children.push_back(element);
  return true;
}

bool ConfigList::Clear() {
  if (type_ != ConfigItem::kList || !data_)
    return false;
  data_->seq_children.clear();
  return true;
}

size_t ConfigList::size() const {
  if(type_ != ConfigItem::kList)
    return 0;
  else
    return data_->seq_children.size();
}

// ConfigMap members

bool ConfigMap::HasKey(const std::string &key) const {
  return bool(Get(key));
}

ConfigItemPtr ConfigMap::Get(const std::string &key) const {
  if (type_ != ConfigItem::kMap || !data_)
    return ConfigItemPtr();
  ConfigItemData::Map::const_iterator it = data_->map_children.find(key);
  if (it == data_->map_children.end())
    return ConfigItemPtr();
  else
    return it->second;
}

bool ConfigMap::Set(const std::string &key, const ConfigItemPtr element) {
  if (type_ != ConfigItem::kMap || !data_)
    return false;
  data_->map_children[key] = element;
  return true;
}

bool ConfigMap::Clear() {
  if (type_ != ConfigItem::kMap || !data_)
    return false;
  data_->map_children.clear();
  return true;
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
  ConfigItemPtr p = data_->Traverse(key);
  return p && p->GetBool(value);
}

bool Config::GetInt(const std::string& key, int *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p = data_->Traverse(key);
  return p && p->GetInt(value);
}

bool Config::GetDouble(const std::string& key, double *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p = data_->Traverse(key);
  return p && p->GetDouble(value);
}

bool Config::GetString(const std::string& key, std::string *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p = data_->Traverse(key);
  return p && p->GetString(value);
}

shared_ptr<ConfigList> Config::GetList(const std::string& key) {
  EZLOGGERVAR(key);
  return dynamic_pointer_cast<ConfigList>(data_->Traverse(key));
}

shared_ptr<ConfigMap> Config::GetMap(const std::string& key) {
  EZLOGGERVAR(key);
  return dynamic_pointer_cast<ConfigMap>(data_->Traverse(key));
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
  root_ = ConvertFromYaml(doc);
  return true;
}

bool ConfigData::SaveToFile(const std::string& file_name) {
  std::ofstream out(file_name.c_str());
  YAML::Emitter emitter;
  EmitYaml(root_, &emitter);
  out << emitter.c_str();
  return true;
}

ConfigItemPtr ConfigData::Traverse(const std::string &key) {
  EZDBGONLYLOGGERPRINT("traverse: %s", key.c_str());
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  // find the YAML::Node, and wrap it!
  ConfigItemPtr p = root_;
  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator end = keys.end();
  for (; it != end; ++it) {
    EZDBGONLYLOGGERPRINT("key node: %s", it->c_str());
    if (!p || p->type() != ConfigItem::kMap)
      return ConfigItemPtr();
    p = dynamic_pointer_cast<ConfigMap>(p)->Get(*it);
  }
  return p;
}

ConfigItemPtr ConfigData::ConvertFromYaml(const YAML::Node &node) {
  if (YAML::NodeType::Null == node.Type()) {
    return ConfigItemPtr();
  }
  if (YAML::NodeType::Scalar == node.Type()) {
    ConfigItemData *data = new ConfigItemData;
    data->value = node.to<std::string>();
    return ConfigItemPtr(new ConfigItem(ConfigItem::kScalar, data));
  }
  if (YAML::NodeType::Sequence == node.Type()) {
    ConfigItemData *data = new ConfigItemData;
    YAML::Iterator it = node.begin();
    YAML::Iterator end = node.end();
    for ( ; it != end; ++it) {
      data->seq_children.push_back(ConvertFromYaml(*it));
    }
    return ConfigItemPtr(new ConfigList(data));
  }
  else if (YAML::NodeType::Map == node.Type()) {
    ConfigItemData *data = new ConfigItemData;
    YAML::Iterator it = node.begin();
    YAML::Iterator end = node.end();
    for ( ; it != end; ++it) {
      std::string key = it.first().to<std::string>();
      data->map_children[key] = ConvertFromYaml(it.second());
    }
    return ConfigItemPtr(new ConfigMap(data));
  }
  return ConfigItemPtr();
}

void ConfigData::EmitYaml(const ConfigItemPtr &node, YAML::Emitter *emitter) {
  if (!node || !emitter) return;
  if (node->type() == ConfigItem::kScalar) {
    *emitter << node->data()->value;
  }
  else if (node->type() == ConfigItem::kList) {
    ConfigItemData::Sequence::const_iterator it = node->data()->seq_children.begin();
    ConfigItemData::Sequence::const_iterator end = node->data()->seq_children.end();
    *emitter << YAML::BeginSeq;
    for ( ; it != end; ++it) {
      EmitYaml(*it, emitter);
    }
    *emitter << YAML::EndSeq;
  }
  else if (node->type() == ConfigItem::kMap) {
    ConfigItemData::Map::const_iterator it = node->data()->map_children.begin();
    ConfigItemData::Map::const_iterator end = node->data()->map_children.end();
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
