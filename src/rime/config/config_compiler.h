//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_CONFIG_COMPILER_H_
#define RIME_CONFIG_COMPILER_H_

#include <rime/common.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

struct ConfigResource : ConfigItemRef {
  string resource_id;
  an<ConfigData> data;
  bool loaded = false;

  ConfigResource(const string& _id, an<ConfigData> _data)
      : ConfigItemRef(nullptr), resource_id(_id), data(_data) {
  }
  an<ConfigItem> GetItem() const override {
    return data->root;
  }
  void SetItem(an<ConfigItem> item) override {
    data->root = item;
  }
};

struct Reference {
  string resource_id;
  string local_path;
  bool optional;

  string repr() const;
};

class ResourceResolver;
struct Dependency;
struct ConfigDependencyGraph;

class ConfigCompiler {
 public:
  static constexpr const char* INCLUDE_DIRECTIVE = "__include";
  static constexpr const char* PATCH_DIRECTIVE = "__patch";

  explicit ConfigCompiler(ResourceResolver* resource_resolver);
  virtual ~ConfigCompiler();

  Reference CreateReference(const string& qualified_path);
  void AddDependency(an<Dependency> dependency);
  void Push(an<ConfigList> config_list, size_t index);
  void Push(an<ConfigMap> config_map, const string& key);
  bool Parse(const string& key, const an<ConfigItem>& item);
  void Pop();

  an<ConfigResource> GetCompiledResource(const string& resource_id) const;
  an<ConfigResource> Compile(const string& file_name);
  bool Link(an<ConfigResource> target);

  bool blocking(const string& full_path) const;
  bool pending(const string& full_path) const;
  bool resolved(const string& full_path) const;
  bool ResolveDependencies(const string& path);

 private:
  ResourceResolver* resource_resolver_;
  the<ConfigDependencyGraph> graph_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPILER_H_
