//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_CONFIG_COMPILER_H_
#define RIME_CONFIG_COMPILER_H_

#include <ostream>
#include <rime/common.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

struct ConfigResource : ConfigItemRef {
  string resource_id;
  an<ConfigData> data;
  bool loaded = false;

  ConfigResource(string_view _id, an<ConfigData> _data)
      : ConfigItemRef(nullptr), resource_id(_id), data(_data) {}
  an<ConfigItem> GetItem() const override { return data->root; }
  void SetItem(an<ConfigItem> item) override { data->root = item; }
};

struct Reference {
  string resource_id;
  string local_path;
  bool optional;

  string repr() const;
};

std::ostream& operator<<(std::ostream& stream, const Reference& reference);

class ConfigCompilerPlugin;
class ResourceResolver;
struct Dependency;
struct ConfigDependencyGraph;

class ConfigCompiler {
 public:
  static constexpr const char* INCLUDE_DIRECTIVE = "__include";
  static constexpr const char* PATCH_DIRECTIVE = "__patch";
  static constexpr const char* APPEND_DIRECTIVE = "__append";
  static constexpr const char* MERGE_DIRECTIVE = "__merge";

  ConfigCompiler(ResourceResolver* resource_resolver,
                 ConfigCompilerPlugin* plugin);
  virtual ~ConfigCompiler();

  Reference CreateReference(string_view qualified_path);
  void AddDependency(an<Dependency> dependency);
  void Push(an<ConfigResource> resource);
  void Push(an<ConfigList> config_list, size_t index);
  void Push(an<ConfigMap> config_map, string_view key);
  bool Parse(string_view key, const an<ConfigItem>& item);
  void Pop();

  void EnumerateResources(
      function<void(an<ConfigResource> resource)> process_resource);
  an<ConfigResource> GetCompiledResource(string_view resource_id) const;
  an<ConfigResource> Compile(string_view file_name);
  bool Link(an<ConfigResource> target);

  bool blocking(string_view full_path) const;
  bool pending(string_view full_path) const;
  bool resolved(string_view full_path) const;
  vector<of<Dependency>> GetDependencies(string_view path);
  bool ResolveDependencies(string_view path);

 private:
  ResourceResolver* resource_resolver_;
  ConfigCompilerPlugin* plugin_;
  the<ConfigDependencyGraph> graph_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPILER_H_
