//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIG_COMPONENT_H_
#define RIME_CONFIG_COMPONENT_H_

#include <iostream>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/config/config_types.h>
#include <rime/resource.h>

namespace rime {

class ConfigData;

class Config : public Class<Config, string_view>, public ConfigItemRef {
 public:
  // CAVEAT: Config instances created without argument will NOT
  // be managed by ConfigComponent
  RIME_API Config();
  RIME_API virtual ~Config();
  // instances of Config with identical config id share a copy of config data
  // in the ConfigComponent
  explicit Config(an<ConfigData> data);

  // returns whether actually saved to file.
  bool Save();
  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  RIME_API bool LoadFromFile(const path& file_path);
  RIME_API bool SaveToFile(const path& file_path);

  // access a tree node of a particular type with "path/to/node"
  RIME_API bool IsNull(string_view path);
  bool IsValue(string_view path);
  RIME_API bool IsList(string_view path);
  RIME_API bool IsMap(string_view path);
  RIME_API bool GetBool(string_view path, bool* value);
  RIME_API bool GetInt(string_view path, int* value);
  RIME_API bool GetDouble(string_view path, double* value);
  RIME_API bool GetString(string_view path, string* value);
  RIME_API size_t GetListSize(string_view path);

  an<ConfigItem> GetItem(string_view path);
  an<ConfigValue> GetValue(string_view path);
  RIME_API an<ConfigList> GetList(string_view path);
  RIME_API an<ConfigMap> GetMap(string_view path);

  // setters
  bool SetBool(string_view path, bool value);
  RIME_API bool SetInt(string_view path, int value);
  bool SetDouble(string_view path, double value);
  RIME_API bool SetString(string_view path, const char* value);
  bool SetString(string_view path, const string& value);
  bool SetString(string_view path, string_view value);
  // setter for adding or replacing items in the tree
  RIME_API bool SetItem(string_view path, an<ConfigItem> item);
  using ConfigItemRef::operator=;

 protected:
  an<ConfigItem> GetItem() const;
  void SetItem(an<ConfigItem> item);

  an<ConfigData> data_;
};

class ConfigCompiler;
class ConfigCompilerPlugin;
struct ConfigResource;

struct ConfigResourceProvider {
  RIME_API static const ResourceType kDefaultResourceType;
  RIME_API static ResourceResolver* CreateResourceResolver(
      const ResourceType& resource_type);
};

struct DeployedConfigResourceProvider {
  RIME_API static const ResourceType kDefaultResourceType;
  RIME_API static ResourceResolver* CreateResourceResolver(
      const ResourceType& resource_type);
};

struct UserConfigResourceProvider {
  RIME_API static const ResourceType kDefaultResourceType;
  RIME_API static ResourceResolver* CreateResourceResolver(
      const ResourceType& resource_type);
};

class ConfigComponentBase : public Config::Component {
 public:
  RIME_API ConfigComponentBase(ResourceResolver* resource_resolver);
  RIME_API virtual ~ConfigComponentBase();
  RIME_API Config* Create(string_view file_name);

 protected:
  virtual an<ConfigData> LoadConfig(string_view config_id) = 0;
  the<ResourceResolver> resource_resolver_;

 private:
  an<ConfigData> GetConfigData(string_view file_name);
  map<string, weak<ConfigData>> cache_;
};

template <class Loader, class ResourceProvider = ConfigResourceProvider>
class ConfigComponent : public ConfigComponentBase {
 public:
  ConfigComponent(const ResourceType& resource_type =
                      ResourceProvider::kDefaultResourceType)
      : ConfigComponentBase(
            ResourceProvider::CreateResourceResolver(resource_type)) {}
  ConfigComponent(function<void(Loader* loader)> setup)
      : ConfigComponentBase(ResourceProvider::CreateResourceResolver(
            ResourceProvider::kDefaultResourceType)) {
    setup(&loader_);
  }

 private:
  an<ConfigData> LoadConfig(string_view config_id) override {
    return loader_.LoadConfig(resource_resolver_.get(), config_id);
  }
  Loader loader_;
};

class ConfigLoader {
 public:
  RIME_API an<ConfigData> LoadConfig(ResourceResolver* resource_resolver,
                                     string_view config_id);
  void set_auto_save(bool auto_save) { auto_save_ = auto_save; }

 private:
  bool auto_save_ = false;
};

class ConfigBuilder {
 public:
  RIME_API ConfigBuilder();
  RIME_API virtual ~ConfigBuilder();
  RIME_API an<ConfigData> LoadConfig(ResourceResolver* resource_resolver,
                                     string_view config_id);
  void InstallPlugin(ConfigCompilerPlugin* plugin);
  bool ApplyPlugins(ConfigCompiler* compiler, an<ConfigResource> resource);

 private:
  vector<the<ConfigCompilerPlugin>> plugins_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPONENT_H_
