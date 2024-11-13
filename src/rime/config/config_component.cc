//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-06 Zou Xu <zouivex@gmail.com>
//

#include <rime/resource.h>
#include <rime/service.h>
#include <rime/config/config_component.h>
#include <rime/config/config_compiler.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>
#include <rime/config/plugins.h>

namespace rime {

Config::Config() : ConfigItemRef(nullptr), data_(New<ConfigData>()) {
  ConfigItemRef::data_ = data_.get();
}

Config::~Config() {}

Config::Config(an<ConfigData> data) : ConfigItemRef(data.get()), data_(data) {}

bool Config::Save() {
  return data_->Save();
}

bool Config::LoadFromStream(std::istream& stream) {
  return data_->LoadFromStream(stream);
}

bool Config::SaveToStream(std::ostream& stream) {
  return data_->SaveToStream(stream);
}

bool Config::LoadFromFile(const path& file_path) {
  return data_->LoadFromFile(file_path, nullptr);
}

bool Config::SaveToFile(const path& file_path) {
  return data_->SaveToFile(file_path);
}

bool Config::IsNull(string_view path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kNull;
}

bool Config::IsValue(string_view path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kScalar;
}

bool Config::IsList(string_view path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kList;
}

bool Config::IsMap(string_view path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kMap;
}

bool Config::GetBool(string_view path, bool* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetBool(value);
}

bool Config::GetInt(string_view path, int* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetInt(value);
}

bool Config::GetDouble(string_view path, double* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetDouble(value);
}

bool Config::GetString(string_view path, string* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetString(value);
}

size_t Config::GetListSize(string_view path) {
  DLOG(INFO) << "read: " << path;
  auto list = GetList(path);
  return list ? list->size() : 0;
}

an<ConfigItem> Config::GetItem(string_view path) {
  DLOG(INFO) << "read: " << path;
  return data_->Traverse(path);
}

an<ConfigValue> Config::GetValue(string_view path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigValue>(data_->Traverse(path));
}

an<ConfigList> Config::GetList(string_view path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigList>(data_->Traverse(path));
}

an<ConfigMap> Config::GetMap(string_view path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigMap>(data_->Traverse(path));
}

bool Config::SetBool(string_view path, bool value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetInt(string_view path, int value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetDouble(string_view path, double value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(string_view path, const char* value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(string_view path, const string& value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(string_view path, string_view value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetItem(string_view path, an<ConfigItem> item) {
  return data_->TraverseWrite(path, item);
}

an<ConfigItem> Config::GetItem() const {
  return data_->root;
}

void Config::SetItem(an<ConfigItem> item) {
  data_->root = item;
  set_modified();
}

const ResourceType ConfigResourceProvider::kDefaultResourceType = {"config", "",
                                                                   ".yaml"};

ResourceResolver* ConfigResourceProvider::CreateResourceResolver(
    const ResourceType& resource_type) {
  return Service::instance().CreateResourceResolver(resource_type);
}

const ResourceType DeployedConfigResourceProvider::kDefaultResourceType = {
    "compiled_config", "", ".yaml"};

ResourceResolver* DeployedConfigResourceProvider::CreateResourceResolver(
    const ResourceType& resource_type) {
  return Service::instance().CreateDeployedResourceResolver(resource_type);
}

const ResourceType UserConfigResourceProvider::kDefaultResourceType = {
    "user_config", "", ".yaml"};

ResourceResolver* UserConfigResourceProvider::CreateResourceResolver(
    const ResourceType& resource_type) {
  return Service::instance().CreateUserSpecificResourceResolver(resource_type);
}

ConfigComponentBase::ConfigComponentBase(ResourceResolver* resource_resolver)
    : resource_resolver_(resource_resolver) {}

ConfigComponentBase::~ConfigComponentBase() {}

Config* ConfigComponentBase::Create(string_view file_name) {
  return new Config(GetConfigData(file_name));
}

an<ConfigData> ConfigComponentBase::GetConfigData(string_view file_name) {
  auto config_id = resource_resolver_->ToResourceId(file_name);
  // keep a weak reference to the shared config data in the component
  weak<ConfigData>& wp(cache_[config_id]);
  if (wp.expired()) {  // create a new copy and load it
    auto data = LoadConfig(config_id);
    wp = data;
    return data;
  }
  // obtain the shared copy
  return wp.lock();
}

an<ConfigData> ConfigLoader::LoadConfig(ResourceResolver* resource_resolver,
                                        string_view config_id) {
  auto data = New<ConfigData>();
  data->LoadFromFile(resource_resolver->ResolvePath(config_id), nullptr);
  data->set_auto_save(auto_save_);
  return data;
}

ConfigBuilder::ConfigBuilder() {}

ConfigBuilder::~ConfigBuilder() {}

void ConfigBuilder::InstallPlugin(ConfigCompilerPlugin* plugin) {
  plugins_.push_back(the<ConfigCompilerPlugin>(plugin));
}

template <class Container>
struct MultiplePlugins : ConfigCompilerPlugin {
  Container& plugins;

  MultiplePlugins(Container& _plugins) : plugins(_plugins) {}
  bool ReviewCompileOutput(ConfigCompiler* compiler,
                           an<ConfigResource> resource) override {
    return ReviewedByAll(&ConfigCompilerPlugin::ReviewCompileOutput, compiler,
                         resource);
  }
  bool ReviewLinkOutput(ConfigCompiler* compiler,
                        an<ConfigResource> resource) override {
    return ReviewedByAll(&ConfigCompilerPlugin::ReviewLinkOutput, compiler,
                         resource);
  }
  typedef bool (ConfigCompilerPlugin::*Reviewer)(ConfigCompiler* compiler,
                                                 an<ConfigResource> resource);
  bool ReviewedByAll(Reviewer reviewer,
                     ConfigCompiler* compiler,
                     an<ConfigResource> resource);
};

template <class Container>
bool MultiplePlugins<Container>::ReviewedByAll(Reviewer reviewer,
                                               ConfigCompiler* compiler,
                                               an<ConfigResource> resource) {
  for (const auto& plugin : plugins) {
    if (!((*plugin).*reviewer)(compiler, resource))
      return false;
  }
  return true;
}

an<ConfigData> ConfigBuilder::LoadConfig(ResourceResolver* resource_resolver,
                                         string_view config_id) {
  MultiplePlugins<decltype(plugins_)> multiple_plugins(plugins_);
  ConfigCompiler compiler(resource_resolver, &multiple_plugins);
  auto resource = compiler.Compile(config_id);
  if (resource->loaded && !compiler.Link(resource)) {
    LOG(ERROR) << "error building config: " << config_id;
  }
  return resource->data;
}

}  // namespace rime
