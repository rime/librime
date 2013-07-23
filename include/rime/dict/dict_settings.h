//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-11-11 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICT_SETTINGS_H_
#define RIME_DICT_SETTINGS_H_

#include <map>
#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

struct DictSettings {
  std::string dict_name;
  std::string dict_version;
  std::string sort_order;
  bool use_preset_vocabulary;
  bool use_rule_based_encoder;
  int max_phrase_length;
  double min_phrase_weight;
  std::vector<std::string> tables;
  std::map<std::string, int> columns;
  DictSettings();
  int GetColumnIndex(const std::string& column_label) const;
  bool LoadFromFile(const std::string& dict_file);
};

}  // namespace rime

#endif  // RIME_DICT_SETTINGS_H_
