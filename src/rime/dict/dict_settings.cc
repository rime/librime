//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-11-11 GONG Chen <chen.sst@gmail.com>
//
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <rime/dict/dict_settings.h>

namespace rime {

DictSettings::DictSettings() {
}

bool DictSettings::LoadDictHeader(std::istream& stream) {
  if (!stream.good()) {
    LOG(ERROR) << "failed to load dict header from stream.";
    return false;
  }
  std::stringstream header;
  string line;
  while (getline(stream, line)) {
    boost::algorithm::trim_right(line);
    header << line << std::endl;
    if (line == "...") {  // yaml doc ending
      break;
    }
  }
  if (!LoadFromStream(header)) {
    return false;
  }
  if ((*this)["name"].IsNull() || (*this)["version"].IsNull()) {
    LOG(ERROR) << "incomplete dict header.";
    return false;
  }
  return true;
}

string DictSettings::dict_name() {
  return (*this)["name"].ToString();
}

string DictSettings::dict_version() {
  return (*this)["version"].ToString();
}

string DictSettings::sort_order() {
  return (*this)["sort"].ToString();
}

bool DictSettings::use_preset_vocabulary() {
  return (*this)["use_preset_vocabulary"].ToBool();
}

bool DictSettings::use_rule_based_encoder() {
  return (*this)["encoder"]["rules"].IsList();
}

int DictSettings::max_phrase_length() {
  return (*this)["max_phrase_length"].ToInt();
}

double DictSettings::min_phrase_weight() {
  return (*this)["min_phrase_weight"].ToDouble();
}

an<ConfigList> DictSettings::GetTables() {
  auto tables = New<ConfigList>();
  tables->Append((*this)["name"]);
  auto imports = (*this)["import_tables"].AsList();
  for (auto it = imports->begin(); it != imports->end(); ++it) {
    if (!Is<ConfigValue>(*it))
      continue;
    string table = As<ConfigValue>(*it)->str();
    if (table == dict_name()) {
      LOG(WARNING) << "cannot import '" << table << "' from itself.";
      continue;
    }
    tables->Append(*it);
  }
  return tables;
}

int DictSettings::GetColumnIndex(const string& column_label) {
  if ((*this)["columns"].IsNull()) {
    // default
    if (column_label == "text") return 0;
    if (column_label == "code") return 1;
    if (column_label == "weight") return 2;
    return -1;
  }
  auto columns = (*this)["columns"].AsList();
  int index = 0;
  for (auto it = columns->begin(); it != columns->end(); ++it, ++index) {
    if (Is<ConfigValue>(*it) && As<ConfigValue>(*it)->str() == column_label) {
      return index;
    }
  }
  return -1;
}

}  // namespace rime
