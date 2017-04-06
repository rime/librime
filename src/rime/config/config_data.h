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

class ConfigItem;

class ConfigData {
 public:
  ConfigData() = default;
  ~ConfigData();

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const string& file_name);
  bool SaveToFile(const string& file_name);
  bool TraverseWrite(const string& key, an<ConfigItem> item);
  an<ConfigItem> Traverse(const string& key);

  bool modified() const { return modified_; }
  void set_modified() { modified_ = true; }

  an<ConfigItem> root;

 protected:
  static an<ConfigItem> ConvertFromYaml(const YAML::Node& yaml_node);
  static void EmitYaml(an<ConfigItem> node,
                       YAML::Emitter* emitter,
                       int depth);
  static void EmitScalar(const string& str_value,
                         YAML::Emitter* emitter);

  string file_name_;
  bool modified_ = false;
};

}  // namespace rime

#endif  // RIME_CONFIG_DATA_H_
