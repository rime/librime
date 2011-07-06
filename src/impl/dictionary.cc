// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#include <yaml-cpp/yaml.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/prism.h>
#include <rime/impl/table.h>

namespace rime {

Dictionary::Dictionary(const std::string &name)
    : name_(name), loaded_(false) {

}

Dictionary::~Dictionary() {
  if (loaded_) {
    Unload();
    loaded_ = false;
  }
}

bool Dictionary::Compile(const std::string &source_file) {
  EZLOGGERFUNCTRACKER;
  YAML::Node doc;
  {
    std::ifstream fin(source_file.c_str());
    YAML::Parser parser(fin);
    parser.GetNextDocument(doc);
  }
  if (doc.Type() != YAML::NodeType::Map) {
    return false;
  }
  std::string dict_name;
  std::string dict_version;
  {
    const YAML::Node *name_node = doc.FindValue("name");
    const YAML::Node *version_node = doc.FindValue("version");
    if (!name_node || !version_node) {
      return false;
    }
    *name_node >> dict_name;
    *version_node >> dict_version;
  }
  EZLOGGERVAR(dict_name);
  EZLOGGERVAR(dict_version);

  const YAML::Node *entries = doc.FindValue("entries");
  if (!entries ||
      entries->Type() != YAML::NodeType::Sequence) {
    return false;
  }
  int entry_count = 0;
  for (YAML::Iterator it = entries->begin(); it != entries->end(); ++it) {
    if (it->Type() != YAML::NodeType::Sequence) {
      EZLOGGERPRINT("Invalid entry %d.", entry_count);
      continue;
    }
    std::string word;
    std::string code;
    double weight = 1.0;
    if (it->size() < 2) {
      EZLOGGERPRINT("Invalid entry %d.", entry_count);
      continue;
    }
    (*it)[0] >> word;
    (*it)[1] >> code;
    if (it->size() > 2) {
      (*it)[2] >> weight;
    }
    ++entry_count;
  }
  EZLOGGERVAR(entry_count);
  return false;
}

bool Dictionary::Load() {

  return false;
}

bool Dictionary::Unload() {

  return false;
}

}  // namespace rime
