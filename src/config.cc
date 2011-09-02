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
#include <yaml-cpp/yaml.h>
#include <rime/config.h>

namespace rime {

// private classes

// TODO:
class ConfigItemData {
 public:
  ConfigItemData() : node_(NULL) {}
  ConfigItemData(const YAML::Node *node) : node_(node) {}
  const YAML::Node* node() const { return node_; }
 private:
  const YAML::Node *node_;
};

class ConfigData {
 public:
  const YAML::Node* Traverse(const std::string &key);
  YAML::Node& doc() { return doc_; }

  static const ConfigItemPtr Convert(const YAML::Node *node);

 private:
  YAML::Node doc_;
};

// ConfigItem members

ConfigItem::~ConfigItem() {
  if (data_) {
    delete data_;
    data_ = NULL;
  }
}

bool ConfigItem::GetBool(bool *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = data_->node()->to<bool>();
  return true;
}

bool ConfigItem::GetInt(int *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = data_->node()->to<int>();
  return true;
}

bool ConfigItem::GetDouble(double *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = data_->node()->to<double>();
  return true;
}

bool ConfigItem::GetString(std::string *value) const {
  if(!data_ || !data_->node())
    return false;
  *value = data_->node()->to<std::string>();
  return true;
}

void ConfigItem::SetBool(bool value) {
  // TODO: 
}

void ConfigItem::SetInt(int value) {
  // TODO: 
}

void ConfigItem::SetDouble(double value) {
  // TODO: 
}

void ConfigItem::SetString(const std::string &value) {
  // TODO: 
}


// ConfigList members

ConfigItemPtr ConfigList::GetAt(size_t i) {
  const YAML::Node* node = data_->node();
  if(node->Type() == YAML::NodeType::Sequence)
  {
    const YAML::Node* p = node->FindValue(i);
    ConfigItemPtr ptr(ConfigData::Convert(p));
    return ptr;
  }
  else
  {
    return ConfigItemPtr();
  }  
}

void ConfigList::SetAt(size_t i, const ConfigItemPtr element) {
  // TODO: 
}

void ConfigList::Append(const ConfigItemPtr element) {
  // TODO: 
}

void ConfigList::Clear() {
  // TODO: 
}

size_t ConfigList::size() const {
  const YAML::Node* node = data_->node();
  if(node->Type() == YAML::NodeType::Sequence)
    return node->size();
  else
  {
    return 0;
  }
}

// ConfigMap members

bool ConfigMap::HasKey(const std::string &key) const {
  const YAML::Node* node = data_->node();
  if(node->Type() == YAML::NodeType::Map)
  {
    const YAML::Node* p = node->FindValue(key);
    return p != NULL;
  }
  else
  {
    return false;
  }
}

ConfigItemPtr ConfigMap::Get(const std::string &key) {
  const YAML::Node* node = data_->node();
  if(node->Type() == YAML::NodeType::Map)
  {
    const YAML::Node* p = node->FindValue(key.c_str());
    return ConfigItemPtr(ConfigData::Convert(p));
  }
  else
  {
    return ConfigItemPtr();
  }
}
  
void ConfigMap::Set(const std::string &key, const ConfigItemPtr element) {
  // TODO:
}

void ConfigMap::Clear() {
  // TODO:
}

// Config members

Config::Config() : data_(new ConfigData) {
}

Config::~Config() {
}

Config::Config(const std::string &file_name) : data_(new ConfigData) {
  LoadFromFile(file_name);
}

bool Config::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  return parser.GetNextDocument(data_->doc());
}

bool Config::SaveToFile(const std::string& file_name) {
  // TODO(zouxu):
  return false;
}

bool Config::IsNull(const std::string &key) {
  EZLOGGERVAR(key);
  const YAML::Node *p = data_->Traverse(key);
  return !p || p->Type() == YAML::NodeType::Null;
}

bool Config::GetBool(const std::string& key, bool *value) {
  EZLOGGERVAR(key);
  const YAML::Node *p = data_->Traverse(key);
  if (!p || p->Type() != YAML::NodeType::Scalar)
    return false;
  *value = p->to<bool>();
  return true;
}

bool Config::GetInt(const std::string& key, int *value) {
  EZLOGGERVAR(key);
  const YAML::Node *p = data_->Traverse(key);
  if (!p || p->Type() != YAML::NodeType::Scalar)
    return false;
  *value = p->to<int>();
  return true;
}

bool Config::GetDouble(const std::string& key, double *value) {
  EZLOGGERVAR(key);
  const YAML::Node *p = data_->Traverse(key);
  if (!p || p->Type() != YAML::NodeType::Scalar)
    return false;
  *value = p->to<double>();
  return true;
}

bool Config::GetString(const std::string& key, std::string *value) {
  EZLOGGERVAR(key);
  const YAML::Node *p = data_->Traverse(key);
  if (!p || p->Type() != YAML::NodeType::Scalar)
    return false;
  *value = p->to<std::string>();
  return true;
}

shared_ptr<ConfigList> Config::GetList(const std::string& key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(ConfigData::Convert(data_->Traverse(key)));
  return dynamic_pointer_cast<ConfigList>(p);
}

shared_ptr<ConfigMap> Config::GetMap(const std::string& key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(ConfigData::Convert(data_->Traverse(key)));
  return dynamic_pointer_cast<ConfigMap>(p);
}

// ConfigComponent members

Config* ConfigComponent::Create(const std::string &config_id) {
  std::string path(boost::str(boost::format(pattern_) % config_id));
  EZLOGGERPRINT("config file path: %s", path.c_str());
  return new Config(path);
}

// ConfigData members

const ConfigItemPtr ConfigData::Convert(const YAML::Node *node) {
  if (!node)
    return ConfigItemPtr();
  // no need to recursively convert YAML::Node structure,
  // just wrap the node itself...
  // we can wrap its children nodes when they are retrived via getters
  YAML::NodeType::value type = node->Type();
  if (type == YAML::NodeType::Scalar) {
    return ConfigItemPtr(new ConfigItem(ConfigItem::kScalar,
                                        new ConfigItemData(node)));
  }
  if (type == YAML::NodeType::Sequence) {
    EZDBGONLYLOGGERPRINT("sequence size: %d", node->size());
    return ConfigItemPtr(new ConfigList(new ConfigItemData(node)));
  }
  if (type == YAML::NodeType::Map) {
    return ConfigItemPtr(new ConfigMap(new ConfigItemData(node)));
  }
  return ConfigItemPtr();
}

const YAML::Node* ConfigData::Traverse(const std::string &key) {
  EZDBGONLYLOGGERPRINT("traverse: %s", key.c_str());
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  // find the YAML::Node, and wrap it!
  const YAML::Node *p = &doc_;
  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator end = keys.end();
  for (; it != end; ++it) {
    EZDBGONLYLOGGERPRINT("key node: %s", it->c_str());
    if (!p || YAML::NodeType::Null == p->Type())
      return NULL;
    p = p->FindValue(*it);
  }
  return p;
}

}  // namespace rime
