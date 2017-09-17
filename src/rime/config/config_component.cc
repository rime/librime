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

Config::Config() : ConfigItemRef(New<ConfigData>()) {
}

Config::~Config() {
}

Config::Config(an<ConfigData> data) : ConfigItemRef(data) {
}

bool Config::LoadFromStream(std::istream& stream) {
  return data_->LoadFromStream(stream);
}

bool Config::SaveToStream(std::ostream& stream) {
  return data_->SaveToStream(stream);
}

bool Config::LoadFromFile(const string& file_name) {
  return data_->LoadFromFile(file_name, nullptr);
}

bool Config::SaveToFile(const string& file_name) {
  return data_->SaveToFile(file_name);
}

bool Config::IsNull(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kNull;
}

bool Config::IsValue(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kScalar;
}

bool Config::IsList(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kList;
}

bool Config::IsMap(const string& path) {
  auto p = data_->Traverse(path);
  return !p || p->type() == ConfigItem::kMap;
}

bool Config::GetBool(const string& path, bool* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetBool(value);
}

bool Config::GetInt(const string& path, int* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetInt(value);
}

bool Config::GetDouble(const string& path, double* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetDouble(value);
}

bool Config::GetString(const string& path, string* value) {
  DLOG(INFO) << "read: " << path;
  auto p = As<ConfigValue>(data_->Traverse(path));
  return p && p->GetString(value);
}

size_t Config::GetListSize(const string& path) {
  DLOG(INFO) << "read: " << path;
  auto list = GetList(path);
  return list ? list->size() : 0;
}

an<ConfigItem> Config::GetItem(const string& path) {
  DLOG(INFO) << "read: " << path;
  return data_->Traverse(path);
}

an<ConfigValue> Config::GetValue(const string& path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigValue>(data_->Traverse(path));
}

an<ConfigList> Config::GetList(const string& path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigList>(data_->Traverse(path));
}

an<ConfigMap> Config::GetMap(const string& path) {
  DLOG(INFO) << "read: " << path;
  return As<ConfigMap>(data_->Traverse(path));
}

bool Config::SetBool(const string& path, bool value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetInt(const string& path, int value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetDouble(const string& path, double value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(const string& path, const char* value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetString(const string& path, const string& value) {
  return SetItem(path, New<ConfigValue>(value));
}

bool Config::SetItem(const string& path, an<ConfigItem> item) {
  return data_->TraverseWrite(path, item);
}

an<ConfigItem> Config::GetItem() const {
  return data_->root;
}

void Config::SetItem(an<ConfigItem> item) {
  data_->root = item;
  set_modified();
}

static const ResourceType kConfigResourceType = {
  "config",
  "",
  ".yaml",
};

ConfigComponent::ConfigComponent()
    : resource_resolver_(
          Service::instance().CreateResourceResolver(kConfigResourceType)) {
}

ConfigComponent::~ConfigComponent() {
}

Config* ConfigComponent::Create(const string& file_name) {
  return new Config(GetConfigData(file_name));
}

void ConfigComponent::InstallPlugin(ConfigCompilerPlugin* plugin) {
  plugins_.push_back(the<ConfigCompilerPlugin>(plugin));
}

template <class Container>
struct MultiplePlugins : ConfigCompilerPlugin {
  Container& plugins;

  MultiplePlugins(Container& _plugins)
      : plugins(_plugins) {
  }
  bool ReviewCompileOutput(ConfigCompiler* compiler,
                           an<ConfigResource> resource) override {
    return ReviewedByAll(&ConfigCompilerPlugin::ReviewCompileOutput,
                         compiler, resource);
  }
  bool ReviewLinkOutput(ConfigCompiler* compiler,
                        an<ConfigResource> resource) override {
    return ReviewedByAll(&ConfigCompilerPlugin::ReviewLinkOutput,
                         compiler, resource);
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
    if(!((*plugin).*reviewer)(compiler, resource))
      return false;
  }
  return true;
}

an<ConfigData> ConfigComponent::GetConfigData(const string& file_name) {
  auto config_id = resource_resolver_->ToResourceId(file_name);
  // keep a weak reference to the shared config data in the component
  weak<ConfigData>& wp(cache_[config_id]);
  if (wp.expired()) {  // create a new copy and load it
    MultiplePlugins<decltype(plugins_)> multiple_plugins(plugins_);
    ConfigCompiler compiler(resource_resolver_.get(), &multiple_plugins);
    auto resource = compiler.Compile(file_name);
    if (resource->loaded && !compiler.Link(resource)) {
      LOG(ERROR) << "error loading config from: " << file_name;
    }
    wp = resource->data;
    return resource->data;
  }
  // obtain the shared copy
  return wp.lock();
}

}  // namespace rime
