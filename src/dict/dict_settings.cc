//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-11-11 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <rime/dict/dict_settings.h>

namespace rime {

static void DiscoverColumns(DictSettings* settings, const YAML::Node& doc);

DictSettings::DictSettings()
    : use_preset_vocabulary(false)
    , max_phrase_length(0)
    , min_phrase_weight(0)
{
}

bool DictSettings::LoadFromFile(const std::string& dict_file) {
  YAML::Node doc = YAML::LoadFile(dict_file);
  if (doc.Type() != YAML::NodeType::Map) {
    LOG(ERROR) << "invalid yaml doc in '" << dict_file << "'.";
    return false;
  }
  if (!doc["name"] || !doc["version"]) {
    LOG(ERROR) << "incomplete dict info in '" << dict_file << "'.";
    return false;
  }
  dict_name = doc["name"].as<std::string>();
  dict_version = doc["version"].as<std::string>();
  if (doc["sort"]) {
    sort_order = doc["sort"].as<std::string>();
  }
  if (doc["use_preset_vocabulary"]) {
    use_preset_vocabulary = doc["use_preset_vocabulary"].as<bool>();
    if (doc["max_phrase_length"])
      max_phrase_length = doc["max_phrase_length"].as<int>();
    if (doc["min_phrase_weight"])
      min_phrase_weight = doc["min_phrase_weight"].as<double>();
  }
  DiscoverColumns(this, doc);
  return true;
}

static void DiscoverColumns(DictSettings* settings, const YAML::Node& doc) {
  if (!settings) return;
  settings->columns.clear();
  YAML::Node columns = doc["columns"];
  if (columns) {
    // user defined column order
    for (YAML::const_iterator it = columns.begin(); it != columns.end(); ++it) {
      std::string column_label = it->as<std::string>();
      settings->columns.push_back(column_label);
    }
  }
  else {
    // default
    //settings->columns.push_back("text");
    //settings->columns.push_back("code");
    //settings->columns.push_back("weight");
  }
}

}  // namespace rime
