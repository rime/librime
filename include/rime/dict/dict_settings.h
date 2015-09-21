//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-11-11 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICT_SETTINGS_H_
#define RIME_DICT_SETTINGS_H_

#include <istream>
#include <rime/common.h>
#include <rime/config.h>

namespace rime {

class DictSettings : public Config {
 public:
  DictSettings();
  bool LoadDictHeader(std::istream& stream);
  string dict_name();
  string dict_version();
  string sort_order();
  bool use_preset_vocabulary();
  bool use_rule_based_encoder();
  int max_phrase_length();
  double min_phrase_weight();
  an<ConfigList> GetTables();
  int GetColumnIndex(const string& column_label);
};

}  // namespace rime

#endif  // RIME_DICT_SETTINGS_H_
