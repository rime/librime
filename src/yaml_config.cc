// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#include <fstream>
#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>
#include "yaml_config.h"
#include <boost/algorithm/string.hpp>
#include <vector>

namespace rime {

void YamlConfig::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  parser.GetNextDocument(doc_);
  // TODO(zouxu):
  // note that the config file can be modified.
  // what's the best chance to traverse the tree, and
  // how to represent read/modified data?
}

void YamlConfig::SaveToFile(const std::string& file_name) {
  // TODO(zouxu):
}

bool YamlConfig::IsNull(const std::string &key) {
  // TODO(zouxu):
  EZLOGGER("IsNull(", key, ")");
  const YAML::Node* p = Traverse(key);
  if(p)
    return true;
  else
    return false;
}

bool YamlConfig::GetBool(const std::string& key, bool *value) {
  // TODO(zouxu):
  EZLOGGER(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<bool>();
  return true;
}

bool YamlConfig::GetInt(const std::string& key, int *value) {
  // TODO(zouxu):
  EZLOGGER(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<int>();
  return true;
}

bool YamlConfig::GetDouble(const std::string& key, double *value) {
  // TODO(zouxu):
  EZLOGGER(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<double>();
  return true;
}

bool YamlConfig::GetString(const std::string& key, std::string *value) {
  // TODO(zouxu):
  EZLOGGER(key);
  const YAML::Node* p = Traverse(key);
  if(!p)
    return false;
  *value = p->to<std::string>();
  return true;
}

ConfigList* YamlConfig::GetList(const std::string& key) {
  // TODO(zouxu):
  return NULL;
}

ConfigMap* YamlConfig::GetMap(const std::string& key) {
  // TODO(zouxu):
  return NULL;
}

YamlConfig* YamlConfigComponent::Create(const std::string &file_name) {
  boost::filesystem::path p(conf_dir_);
  p /= file_name;
  EZLOGGER("yaml config path:", p.string());
  return new YamlConfig(p.string());
}

const YAML::Node* YamlConfig::Traverse(const std::string& key){
  std::vector<std::string> keys;
  boost::split(keys, key, boost::is_any_of("/"));

  std::vector<std::string>::iterator it = keys.begin();
  std::vector<std::string>::iterator ite = keys.end();

  const YAML::Node* pNode = &doc_; 

  for(; it != ite; ++it){
    EZLOGGER("keyNode:", *it);
    pNode = pNode->FindValue(*it);
    if(!pNode)
      return NULL;
  }
  return pNode;
}
}  // namespace rime
