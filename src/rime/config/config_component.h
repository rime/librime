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

  // access a tree node of a particular type with "path/to/node"
  bool IsNull(const string& path);
  bool IsValue(const string& path);
  bool IsList(const string& path);
  bool IsMap(const string& path);
  bool GetBool(const string& path, bool* value);
  bool GetInt(const string& path, int* value);
  bool GetDouble(const string& path, double* value);
  bool GetString(const string& path, string* value);
  size_t GetListSize(const string& path);

  an<ConfigItem> GetItem(const string& path);
  an<ConfigValue> GetValue(const string& path);
  an<ConfigList> GetList(const string& path);
  an<ConfigMap> GetMap(const string& path);

  // setters
  bool SetBool(const string& path, bool value);
  bool SetInt(const string& path, int value);
  bool SetDouble(const string& path, double value);
  bool SetString(const string& path, const char* value);
  bool SetString(const string& path, const string& value);
  // setter for adding or replacing items in the tree
  bool SetItem(const string& path, an<ConfigItem> item);
  using ConfigItemRef::operator=;

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
