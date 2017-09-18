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

class ConfigData;

class Config : public Class<Config, const string&>, public ConfigItemRef {
 public:
  // CAVEAT: Config instances created without argument will NOT
  // be managed by ConfigComponent
  RIME_API Config();
  RIME_API virtual ~Config();
  // instances of Config with identical config id share a copy of config data
  // in the ConfigComponent
  explicit Config(an<ConfigData> data);

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  RIME_API bool LoadFromFile(const string& file_name);
  RIME_API bool SaveToFile(const string& file_name);

  // access a tree node of a particular type with "path/to/node"
  RIME_API bool IsNull(const string& path);
  bool IsValue(const string& path);
  RIME_API bool IsList(const string& path);
  RIME_API bool IsMap(const string& path);
  RIME_API bool GetBool(const string& path, bool* value);
  RIME_API bool GetInt(const string& path, int* value);
  RIME_API bool GetDouble(const string& path, double* value);
  RIME_API bool GetString(const string& path, string* value);
  RIME_API size_t GetListSize(const string& path);

  an<ConfigItem> GetItem(const string& path);
  an<ConfigValue> GetValue(const string& path);
  RIME_API an<ConfigList> GetList(const string& path);
  RIME_API an<ConfigMap> GetMap(const string& path);

  // setters
  bool SetBool(const string& path, bool value);
  RIME_API bool SetInt(const string& path, int value);
  bool SetDouble(const string& path, double value);
  RIME_API bool SetString(const string& path, const char* value);
  bool SetString(const string& path, const string& value);
  // setter for adding or replacing items in the tree
  RIME_API bool SetItem(const string& path, an<ConfigItem> item);
  using ConfigItemRef::operator=;

 protected:
  an<ConfigItem> GetItem() const;
  void SetItem(an<ConfigItem> item);
};

class ConfigCompiler;
class ConfigCompilerPlugin;
class ResourceResolver;
struct ConfigResource;

class ConfigComponent : public Config::Component {
 public:
  RIME_API ConfigComponent();
  ~ConfigComponent();
  Config* Create(const string& file_name);
  void InstallPlugin(ConfigCompilerPlugin *plugin);
  bool ApplyPlugins(ConfigCompiler* compiler, an<ConfigResource> resource);

private:
  an<ConfigData> GetConfigData(const string& file_name);
  map<string, weak<ConfigData>> cache_;
  the<ResourceResolver> resource_resolver_;
  vector<the<ConfigCompilerPlugin>> plugins_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPONENT_H_
