//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIG_COMPONENT_H_
#define RIME_CONFIG_COMPONENT_H_

#include <iostream>
#include <type_traits>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/config/config_types.h>

namespace rime {

class Config : public Class<Config, const string&>, public ConfigItemRef {
 public:
  // CAVEAT: Config instances created without argument will NOT
  // be managed by ConfigComponent
  Config();
  virtual ~Config();
  // instances of Config with identical file_name share a copy of config data
  // that could be reloaded by ConfigComponent once notified changes to the file
  explicit Config(const string& file_name);

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const string& file_name);
  bool SaveToFile(const string& file_name);

  // access a tree node of a particular type with "path/to/key"
  bool IsNull(const string& key);
  bool IsValue(const string& key);
  bool IsList(const string& key);
  bool IsMap(const string& key);
  bool GetBool(const string& key, bool* value);
  bool GetInt(const string& key, int* value);
  bool GetDouble(const string& key, double* value);
  bool GetString(const string& key, string* value);

  an<ConfigItem> GetItem(const string& key);
  an<ConfigValue> GetValue(const string& key);
  an<ConfigList> GetList(const string& key);
  an<ConfigMap> GetMap(const string& key);

  // setters
  bool SetBool(const string& key, bool value);
  bool SetInt(const string& key, int value);
  bool SetDouble(const string& key, double value);
  bool SetString(const string& key, const char* value);
  bool SetString(const string& key, const string& value);
  // setter for adding / replacing items in the tree
  bool SetItem(const string& key, an<ConfigItem> item);

  template <class T>
  Config& operator= (const T& x) {
    SetItem(AsConfigItem(x, std::is_convertible<T, an<ConfigItem>>()));
    return *this;
  }

 protected:
  an<ConfigItem> GetItem() const;
  void SetItem(an<ConfigItem> item);
};

class ConfigComponent : public Config::Component {
 public:
  ConfigComponent(const string& pattern) : pattern_(pattern) {}
  Config* Create(const string& config_id);
  string GetConfigFilePath(const string& config_id);
  const string& pattern() const { return pattern_; }

 private:
  string pattern_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPONENT_H_
