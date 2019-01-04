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
#include <rime/resource.h>

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
struct ConfigResource;

struct ConfigResourceProvider {
  RIME_API static const ResourceType kDefaultResourceType;
  RIME_API static ResourceResolver*
  CreateResourceResolver(const ResourceType& resource_type);
};

struct UserConfigResourceProvider {
  RIME_API static const ResourceType kDefaultResourceType;
  RIME_API static ResourceResolver*
  CreateResourceResolver(const ResourceType& resource_type);
};

class ConfigComponentBase : public Config::Component {
 public:
  RIME_API ConfigComponentBase(ResourceResolver* resource_resolver);
  RIME_API virtual ~ConfigComponentBase();
  RIME_API Config* Create(const string& file_name);

 protected:
  virtual an<ConfigData> LoadConfig(const string& config_id) = 0;
  the<ResourceResolver> resource_resolver_;

 private:
  an<ConfigData> GetConfigData(const string& file_name);
  map<string, weak<ConfigData>> cache_;
};

template <class Loader, class ResourceProvider = ConfigResourceProvider>
    class ConfigComponent : public ConfigComponentBase {
 public:
  ConfigComponent(const ResourceType& resource_type =
                  ResourceProvider::kDefaultResourceType)
      : ConfigComponentBase(
            ResourceProvider::CreateResourceResolver(resource_type)) {}
  ConfigComponent(function<void (Loader* loader)> setup)
      : ConfigComponentBase(
            ResourceProvider::CreateResourceResolver(
                ResourceProvider::kDefaultResourceType)) {
    setup(&loader_);
  }
 private:
  an<ConfigData> LoadConfig(const string& config_id) override {
    return loader_.LoadConfig(resource_resolver_.get(), config_id);
  }
  Loader loader_;
};

class ConfigLoader {
 public:
  RIME_API an<ConfigData> LoadConfig(ResourceResolver* resource_resolver,
                                     const string& config_id);
  void set_auto_save(bool auto_save) { auto_save_ = auto_save; }
 private:
  bool auto_save_ = false;
};

class ConfigBuilder {
 public:
  RIME_API ConfigBuilder();
  RIME_API virtual ~ConfigBuilder();
  RIME_API an<ConfigData> LoadConfig(ResourceResolver* resource_resolver,
                                     const string& config_id);
  void InstallPlugin(ConfigCompilerPlugin *plugin);
  bool ApplyPlugins(ConfigCompiler* compiler, an<ConfigResource> resource);
 private:
  vector<the<ConfigCompilerPlugin>> plugins_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPONENT_H_
