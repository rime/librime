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

#include <string>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {  

class Config : public Class_<Config, const std::string&> {
 public:
  virtual const std::string GetValue(const std::string &key) = 0;
};

class YamlConfig : public Config {
 public:
  YamlConfig() {}
  virtual ~YamlConfig() {}
  YamlConfig(const std::string &file_name) {
    LoadFromFile(file_name);
  }

  void LoadFromFile(const std::string& file_name);
  void SaveToFile(const std::string& file_name);
  const std::string GetValue(const std::string& key);

 private:
  YAML::Node doc_;
};

class YamlConfigComponent : public Component_<YamlConfig> {
 public:
  YamlConfigComponent(const std::string &conf_dir) : conf_dir_(conf_dir) {}
  YamlConfig* Create(const std::string &file_name);
  const std::string& conf_dir() const { return conf_dir_; }

 private:
  std::string conf_dir_;
};

}  // namespace rime

#endif
