// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_YAML_CONFIG_H_
#define RIME_YAML_CONFIG_H_

#include <string>
#include <yaml-cpp/yaml.h>
#include <rime/component.h>
#include <rime/config.h>

namespace rime {  

class YamlConfig : public Config {
 public:
  YamlConfig() {}
  virtual ~YamlConfig() {}
  YamlConfig(const std::string &file_name) {
    LoadFromFile(file_name);
  }

  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);

  virtual bool IsNull(const std::string &key);
  virtual bool GetBool(const std::string &key, bool *value);
  virtual bool GetInt(const std::string &key, int *value);
  virtual bool GetDouble(const std::string &key, double *value);
  virtual bool GetString(const std::string &key, std::string *value);
  virtual ConfigList* GetList(const std::string &key);
  virtual ConfigMap* GetMap(const std::string &key);

 private:
  YAML::Node doc_;
  const YAML::Node* Traverse(const std::string &key);
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

#endif  // RIME_YAML_CONFIG_H_
