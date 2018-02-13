//
// Copyright RIME Developers
// Distributed under the BSD License
//

#ifndef RIME_CONFIG_DATA_H_
#define RIME_CONFIG_DATA_H_

#include <iostream>
#include <yaml-cpp/yaml.h>
#include <rime/common.h>

namespace rime {

class ConfigCompiler;
class ConfigItem;

class ConfigData {
 public:
  ConfigData() = default;
  ~ConfigData();

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const string& file_name, ConfigCompiler* compiler);
  bool SaveToFile(const string& file_name);
  bool TraverseWrite(const string& path, an<ConfigItem> item);
  an<ConfigItem> Traverse(const string& path);

  static vector<string> SplitPath(const string& path);
  static string JoinPath(const vector<string>& keys);
  static bool IsListItemReference(const string& key);
  static string FormatListIndex(size_t index);
  static size_t ResolveListIndex(an<ConfigItem> list, const string& key,
                                 bool read_only = false);

  const string& file_name() const { return file_name_; }
  bool modified() const { return modified_; }
  void set_modified() { modified_ = true; }
  void set_auto_save(bool auto_save) { auto_save_ = auto_save; }

  an<ConfigItem> root;

 protected:
  static an<ConfigItem> ConvertFromYaml(const YAML::Node& yaml_node,
                                        ConfigCompiler* compiler);
  static void EmitYaml(an<ConfigItem> node,
                       YAML::Emitter* emitter,
                       int depth);
  static void EmitScalar(const string& str_value,
                         YAML::Emitter* emitter);

  string file_name_;
  bool modified_ = false;
  bool auto_save_ = false;
};

}  // namespace rime

#endif  // RIME_CONFIG_DATA_H_
