// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIG_H_
#define RIME_CONFIG_H_

#include <rime/common.h>
#include <rime/component.h>
#include <yaml-cpp/yaml.h>

namespace rime {  

class Config : public Component {
 public:
  Config() {}
  virtual ~Config() {}
  void LoadFromFile(const std::string& file_name);
  void SaveToFile(const std::string& file_name);
  std::string GetValue(const std::string& key_path);
 private:
  YAML::Node doc_;
};

class ConfigClass : public ComponentClass {
 public:
  virtual shared_ptr<Component> CreateInstance(Engine *engine) {
    return shared_ptr<Component>(new Config);
  }
  virtual const std::string name() const { return "config"; }
};

}  // namespace rime

#endif
