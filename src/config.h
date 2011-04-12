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

// type of config values
enum ConfigValueType {
  kNull = 0,
  kBool,
  kNumber,
  kString,
  kList,
  kMap,
  kLastType
};

// config item base class
class ConfigItem {
 public:
  virtual bool AsBool() const = 0;
  virtual int AsInt() const = 0;
  virtual double AsDouble() const = 0;
  virtual std::string AsString() const = 0;
  ConfigValueType type() const { return type_; }
  // TODO: static assert that class T inherits from ConfigItem
  template <class T>
  T* As() const { return dynamic_cast<T>(this); }
 protected:
  ConfigValueType type_;
};

class ConfigList : public ConfigItem {
 public:
  virtual ConfigItem* Get(size_t n) const = 0;
  virtual size_t length() const = 0;
};

class ConfigMap : public ConfigItem {
 public:
  virtual bool HasKey(const std::string &key) const = 0;
  virtual ConfigItem* Get(const std::string &key) const = 0;
};

// config component interface
class Config : public Class_<Config, std::string> {
 public:
  virtual const bool GetBool(const std::string &key) = 0;
  virtual const int GetInt(const std::string &key) = 0;
  virtual const double GetDouble(const std::string &key) = 0;
  virtual const std::string GetString(const std::string &key) = 0;
  virtual ConfigList* GetList(const std::string &key) = 0;
  virtual ConfigMap* GetMap(const std::string &key) = 0;
};

// a config implementation using yaml files
class YamlConfig : public Config {
 public:
  YamlConfig() {}
  virtual ~YamlConfig() {}
  YamlConfig(const std::string &file_name) {
    LoadFromFile(file_name);
  }

  void LoadFromFile(const std::string& file_name);
  void SaveToFile(const std::string& file_name);

  virtual const bool GetBool(const std::string &key);
  virtual const int GetInt(const std::string &key);
  virtual const double GetDouble(const std::string &key);
  virtual const std::string GetString(const std::string &key);
  virtual ConfigList* GetList(const std::string &key);
  virtual ConfigMap* GetMap(const std::string &key);

 private:
  YAML::Node doc_;
};

// the component implementation
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
