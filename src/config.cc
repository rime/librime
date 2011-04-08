// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#include <fstream>
#include "config.h"

namespace rime {

void YamlConfig::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  parser.GetNextDocument(doc_);
}

void YamlConfig::SaveToFile(const std::string& file_name) {
}

const std::string YamlConfig::GetValue(const std::string& key) {
  // TODO(zouxu):
  return "";
}

YamlConfig* YamlConfigComponent::Create(const std::string &file_name) {
  // TODO: use boost::filesystem's path join function
  return new YamlConfig(conf_dir_ + file_name);
}

}  // namespace rime
