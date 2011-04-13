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
  return true;  // undefined or null
}

bool YamlConfig::GetBool(const std::string& key, bool *value) {
  // TODO(zouxu):
  return false;
}

bool YamlConfig::GetInt(const std::string& key, int *value) {
  // TODO(zouxu):
  return false;
}

bool YamlConfig::GetDouble(const std::string& key, double *value) {
  // TODO(zouxu):
  return false;
}

bool YamlConfig::GetString(const std::string& key, std::string *value) {
  // TODO(zouxu):
  return false;
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
  return new YamlConfig(p.string());
}

}  // namespace rime
