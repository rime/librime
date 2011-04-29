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
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include "yaml_config.h"

namespace rime {

bool YamlConfig::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  parser.GetNextDocument(doc_);
  return true;
}

bool YamlConfig::SaveToFile(const std::string& file_name) {
  // TODO(zouxu):
  return false;
}

bool YamlConfig::IsNull(const std::string &key) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  return !p;
}

bool YamlConfig::GetBool(const std::string& key, bool *value) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<bool>();
  return true;
}

bool YamlConfig::GetInt(const std::string& key, int *value) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<int>();
  return true;
}

bool YamlConfig::GetDouble(const std::string& key, double *value) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<double>();
  return true;
}

bool YamlConfig::GetString(const std::string& key, std::string *value) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<std::string>();
  return true;
}

ConfigList* YamlConfig::GetList(const std::string& key) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  // TODO(zouxu):
  return NULL;
}

ConfigMap* YamlConfig::GetMap(const std::string& key) {
  EZLOGGERVAR(key);
  const YAML::Node* p = Traverse(key);
  // TODO(zouxu):
  return NULL;
}

YamlConfig* YamlConfigComponent::Create(const std::string &file_name) {
  boost::filesystem::path p(conf_dir_);
  p /= file_name;
  EZLOGGERPRINT("yaml config path: %s", p.string().c_str());
  return new YamlConfig(p.string());
}

const YAML::Node* YamlConfig::Traverse(const std::string &key) {
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));

  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator ite = keys.end();

  const YAML::Node* pNode = &doc_; 

  for(; it != ite; ++it) {
    EZLOGGERPRINT("key node: %s", it->c_str());
    pNode = pNode->FindValue(*it);
    if(!pNode)
      return NULL;
  }
  return pNode;
}

}  // namespace rime
