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
  string name;
  an<ConfigData> data;

  ConfigResource(const string& _name, an<ConfigData> _data)
      : ConfigItemRef(nullptr), name(_name), data(_data) {
  }
  an<ConfigItem> GetItem() const override {
    return data->root;
  }
  void SetItem(an<ConfigItem> item) override {
    data->root = item;
  }
};

struct ConfigDependencyGraph;

class ConfigCompiler {
 public:
  static constexpr const char* INCLUDE_DIRECTIVE = "__include";
  static constexpr const char* PATCH_DIRECTIVE = "__patch";

  ConfigCompiler();
  virtual ~ConfigCompiler();

  void Push(an<ConfigList> config_list, size_t index);
  void Push(an<ConfigMap> config_map, const string& key);
  bool Parse(const string& key, const an<ConfigItem>& item);
  void Pop();

  an<ConfigResource> GetCompiledResource(const string& resource_name) const;
  an<ConfigResource> Compile(const string& resource_name);
  bool Link(an<ConfigResource> target);

  bool blocking(const string& full_path) const;
  bool pending(const string& full_path) const;
  bool resolved(const string& full_path) const;
  bool ResolveDependencies(const string& path);

 private:
  the<ConfigDependencyGraph> graph_;
};

}  // namespace rime

#endif  // RIME_CONFIG_COMPILER_H_
