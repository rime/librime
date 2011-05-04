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
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include "yaml_config.h"

namespace rime {

bool YamlConfig::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  YAML::Node doc;
  parser.GetNextDocument(doc);
  tree_ = Convert(&doc);
  return tree_;
}

bool YamlConfig::SaveToFile(const std::string& file_name) {
  // TODO(zouxu):
  return false;
}

bool YamlConfig::IsNull(const std::string &key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  return !p || ConfigItem::kNull == p->type();
}

bool YamlConfig::GetBool(const std::string& key, bool *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  if(!p)
    return false;
  return p->get<bool>(value);
}

bool YamlConfig::GetInt(const std::string& key, int *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  if(!p)
    return false;
  return p->get<int>(value);
}

bool YamlConfig::GetDouble(const std::string& key, double *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  if(!p)
    return false;
  return p->get<double>(value);
}

bool YamlConfig::GetString(const std::string& key, std::string *value) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  if(!p)
    return false;
  return p->get<std::string>(value);
}

shared_ptr<ConfigList> YamlConfig::GetList(const std::string& key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  return dynamic_pointer_cast<ConfigList>(p);
}

shared_ptr<ConfigMap> YamlConfig::GetMap(const std::string& key) {
  EZLOGGERVAR(key);
  ConfigItemPtr p(Traverse(key));
  return dynamic_pointer_cast<ConfigMap>(p);
}

YamlConfig* YamlConfigComponent::Create(const std::string &file_name) {
  boost::filesystem::path p(conf_dir_);
  p /= file_name;
  EZLOGGERPRINT("yaml config path: %s", p.string().c_str());
  return new YamlConfig(p.string());
}

const ConfigItemPtr YamlConfig::Convert(const YAML::Node *node) {
  // TODO:
  ConfigItemPtr p;
  RecursiveConvert(*node, p);
  return p;
}

void YamlConfig::RecursiveConvert(const YAML::Node &node, ConfigItemPtr& tree_ptr){
  YAML::NodeType::value type = node.Type();
  if(type == YAML::NodeType::Scalar){
    tree_ptr = ConfigValue::Create(node.to<std::string>());
  }
  else if(type == YAML::NodeType::Sequence){
    ConfigList* list_ptr = new ConfigList();
    for (unsigned int i = 0; i < node.size(); i++) {
      const YAML::Node & subnode = node[i];
      ConfigItemPtr config_ptr;
      RecursiveConvert(subnode, config_ptr);
      list_ptr->push_back(config_ptr);
    }
    tree_ptr = ConfigItemPtr(list_ptr);
  }
  else if(type == YAML::NodeType::Map){
    ConfigMap* map_ptr = new ConfigMap();
    for (YAML::Iterator i = node.begin(); i != node.end(); ++i) {
      const YAML::Node & key   = i.first();
      const YAML::Node & value = i.second();
      ConfigItemPtr config_ptr;
      RecursiveConvert(value, config_ptr);
      (*map_ptr)[key.to<std::string>()] = config_ptr;
    }
    tree_ptr = ConfigItemPtr(map_ptr);
  }
  else{
  }
}

const ConfigItemPtr YamlConfig::Traverse(const std::string &key) {
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));

  ConfigItemPtr p(tree_); 
  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator end = keys.end();
  for (; it != end; ++it) {
    EZLOGGERPRINT("key node: %s", it->c_str());
    shared_ptr<ConfigMap> map_ptr(dynamic_pointer_cast<ConfigMap>(p));
    if(!map_ptr)
      return ConfigItemPtr();
    ConfigMap::const_iterator entry = map_ptr->find(*it);
    if (entry == map_ptr->end())
      return ConfigItemPtr();
    p = entry->second;
  }
  return p;
}

}  // namespace rime
