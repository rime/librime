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

void Config::LoadFromFile(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  YAML::Parser parser(fin);
  parser.GetNextDocument(doc_);
}

void Config::SaveToFile(const std::string& file_name) {
}

const std::string Config::GetValue(const std::string& key_path) {
  return "";
}

}  // namespace rime
