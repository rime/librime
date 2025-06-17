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
  RIME_DLL Config();
  RIME_DLL virtual ~Config();
  // instances of Config with identical config id share a copy of config data
  // in the ConfigComponent
  explicit Config(an<ConfigData> data);

  // returns whether actually saved to file.
  bool Save();
  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  RIME_DLL bool LoadFromFile(const path& file_path);
  RIME_DLL bool SaveToFile(const path& file_path);

  // access a tree node of a particular type with "path/to/node"
  RIME_DLL bool IsNull(const string& path);
  bool IsValue(const string& path);
  RIME_DLL bool IsList(const string& path);
  RIME_DLL bool IsMap(const string& path);
  RIME_DLL bool GetBool(const string& path, bool* value);
  RIME_DLL bool GetInt(const string& path, int* value);
  RIME_DLL bool GetDouble(const string& path, double* value);
  RIME_DLL bool GetString(const string& path, string* value);
  RIME_DLL size_t GetListSize(const string& path);

  an<ConfigItem> GetItem(const string& path);
  an<ConfigValue> GetValue(const string& path);
  RIME_DLL an<ConfigList> GetList(const string& path);
  RIME_DLL an<ConfigMap> GetMap(const string& path);

  // setters
  bool SetBool(const string& path, bool value);
  RIME_DLL bool SetInt(const string& path, int value);
  bool SetDouble(const string& path, double value);
  RIME_DLL bool SetString(const string& path, const char* value);
  bool SetString(const string& path, const string& value);
  // setter for adding or replacing items in the tree
  RIME_DLL bool SetItem(const string& path, an<ConfigItem> item);
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
  RIME_DLL static const ResourceType kDefaultResourceType;
  RIME_DLL static ResourceResolver* CreateResourceResolver(
      const ResourceType& resource_type);
};

struct DeployedConfigResourceProvider {
  RIME_DLL static const ResourceType kDefaultResourceType;
  RIME_DLL static ResourceResolver* CreateResourceResolver(
      const ResourceType& resource_type);
};

struct UserConfigResourceProvider {
  RIME_DLL static const ResourceType kDefaultResourceType;
  RIME_DLL static ResourceResolver* CreateResourceResolver(
      const ResourceType& resource_type);
};

class ConfigComponentBase : public Config::Component {
 public:
  RIME_DLL ConfigComponentBase(ResourceResolver* resource_resolver);
  RIME_DLL virtual ~ConfigComponentBase();
  RIME_DLL Config* Create(const string& file_name);

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
  ConfigComponent(function<void(Loader* loader)> setup)
      : ConfigComponentBase(ResourceProvider::CreateResourceResolver(
            ResourceProvider::kDefaultResourceType)) {
    setup(&loader_);
  }
  ConfigComponent(function<void(Loader* loader)> setup,
                  const string& loader_dir)
      : ConfigComponentBase(
            new ResourceResolver(ResourceProvider::kDefaultResourceType)) {
    resource_resolver_->set_root_path(path(loader_dir));
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
  RIME_DLL an<ConfigData> LoadConfig(ResourceResolver* resource_resolver,
                                     const string& config_id);
  void set_auto_save(bool auto_save) { auto_save_ = auto_save; }

 private:
  bool auto_save_ = false;
};

class ConfigBuilder {
 public:
  RIME_DLL ConfigBuilder();
  RIME_DLL virtual ~ConfigBuilder();
  RIME_DLL an<ConfigData> LoadConfig(ResourceResolver* resource_resolver,
                                     const string& config_id);
  void InstallPlugin(ConfigCompilerPlugin* plugin);
  bool ApplyPlugins(ConfigCompiler* compiler, an<ConfigResource> resource);

 private:
  vector<the<ConfigCompilerPlugin>> plugins_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPONENT_H_
