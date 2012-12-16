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

static void DiscoverColumns(DictSettings* settings, const YAML::Node* doc);

DictSettings::DictSettings()
    : use_preset_vocabulary(false)
    , max_phrase_length(0)
    , min_phrase_weight(0)
{
}

bool DictSettings::LoadFromFile(const std::string& dict_file) {
  YAML::Node doc;
  {
    std::ifstream fin(dict_file.c_str());
    YAML::Parser parser(fin);
    if (!parser.GetNextDocument(doc)) {
      LOG(ERROR) << "Error parsing yaml doc in '" << dict_file << "'.";
      return false;
    }
  }
  if (doc.Type() != YAML::NodeType::Map) {
    LOG(ERROR) << "invalid yaml doc in '" << dict_file << "'.";
    return false;
  }
  const YAML::Node *name_node = doc.FindValue("name");
  const YAML::Node *version_node = doc.FindValue("version");
  const YAML::Node *sort_order_node = doc.FindValue("sort");
  const YAML::Node *use_preset_vocabulary_node = doc.FindValue("use_preset_vocabulary");
  const YAML::Node *max_phrase_length_node = doc.FindValue("max_phrase_length");
  const YAML::Node *min_phrase_weight_node = doc.FindValue("min_phrase_weight");
  if (!name_node || !version_node) {
    LOG(ERROR) << "incomplete dict info in '" << dict_file << "'.";
    return false;
  }
  *name_node >> dict_name;
  *version_node >> dict_version;
  if (sort_order_node) {
    *sort_order_node >> sort_order;
  }
  if (use_preset_vocabulary_node) {
    *use_preset_vocabulary_node >> use_preset_vocabulary;
    if (max_phrase_length_node)
      *max_phrase_length_node >> max_phrase_length;
    if (min_phrase_weight_node)
      *min_phrase_weight_node >> min_phrase_weight;
  }
  DiscoverColumns(this, &doc);
  return true;
}

static void DiscoverColumns(DictSettings* settings, const YAML::Node* doc) {
  if (!settings || !doc) return;
  settings->columns.clear();
  const YAML::Node* node = doc->FindValue("columns");
  if (node) {
    // user defined column order
    for (YAML::Iterator it = node->begin(); it != node->end(); ++it) {
      std::string column_label;
      *it >> column_label;
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
