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
#include "config.h"

namespace rime {

void YamlConfig::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  parser.GetNextDocument(doc_);
}

void YamlConfig::SaveToFile(const std::string& file_name) {
}

const bool YamlConfig::GetBool(const std::string& key) {
  // TODO(zouxu):
  return false;
}

const int YamlConfig::GetInt(const std::string& key) {
  // TODO(zouxu):
  return 0;
}

const double YamlConfig::GetDouble(const std::string& key) {
  // TODO(zouxu):
  return 0.0;
}

const std::string YamlConfig::GetString(const std::string& key) {
  // TODO(zouxu):
  return "";
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
