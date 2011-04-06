// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//

#include "config_component.h"
#include <fstream>

namespace rime {

void ConfigComponent::LoadFromFile(const std::string& fileName){
  std::ifstream fin(fileName.c_str());
  YAML::Parser parser(fin);
  parser.GetNextDocument(doc);
}

void ConfigComponent::SaveToFile(const std::string& fileName){
}

std::string ConfigComponent::GetValue(const std::string& keyPath){
  return "";
}

}  // namespace rime