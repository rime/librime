//
// Copyright RIME Developers
// Distributed under the BSD License
//

#ifndef RIME_CONFIG_DATA_H_
#define RIME_CONFIG_DATA_H_

#include <iostream>
#include <rime/common.h>

namespace rime {

class ConfigCompiler;
class ConfigItem;

class ConfigData {
 public:
  ConfigData() = default;
  ~ConfigData();

  // returns whether actually saved to file.
  bool Save();
  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const path& file_path, ConfigCompiler* compiler);
  bool SaveToFile(const path& file_path);
  bool TraverseWrite(string_view path, an<ConfigItem> item);
  an<ConfigItem> Traverse(string_view path);

  static vector<string> SplitPath(string_view path);
  static string JoinPath(const vector<string>& keys);
  static bool IsListItemReference(string_view key);
  static string FormatListIndex(size_t index);
  static size_t ResolveListIndex(an<ConfigItem> list,
                                 string_view key,
                                 bool read_only = false);

  const path& file_path() const { return file_path_; }
  bool modified() const { return modified_; }
  void set_modified() { modified_ = true; }
  void set_auto_save(bool auto_save) { auto_save_ = auto_save; }

  an<ConfigItem> root;

 protected:
  path file_path_;
  bool modified_ = false;
  bool auto_save_ = false;
};

}  // namespace rime

#endif  // RIME_CONFIG_DATA_H_
