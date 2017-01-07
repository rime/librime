//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-06 Zou Xu <zouivex@gmail.com>
//
#include <cstdlib>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <rime/config.h>

namespace rime {

class ConfigData {
 public:
  ConfigData() = default;
  ~ConfigData();

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const string& file_name);
  bool SaveToFile(const string& file_name);
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

class ConfigDataManager : public map<string, weak<ConfigData>> {
 public:
  an<ConfigData> GetConfigData(const string& config_file_path);
  bool ReloadConfigData(const string& config_file_path);

  static ConfigDataManager& instance();

 private:
  ConfigDataManager() = default;
};

// ConfigValue members

ConfigValue::ConfigValue(bool value)
    : ConfigItem(kScalar) {
  SetBool(value);
}

ConfigValue::ConfigValue(int value)
    : ConfigItem(kScalar) {
  SetInt(value);
}

ConfigValue::ConfigValue(double value)
    : ConfigItem(kScalar) {
  SetDouble(value);
}

ConfigValue::ConfigValue(const char* value)
    : ConfigItem(kScalar), value_(value) {
}

ConfigValue::ConfigValue(const string& value)
    : ConfigItem(kScalar), value_(value) {
}

bool ConfigValue::GetBool(bool* value) const {
  if (!value || value_.empty())
    return false;
  string bstr = value_;
  boost::to_lower(bstr);
  if ("true" == bstr) {
    *value = true;
    return true;
  }
  else if ("false" == bstr) {
    *value = false;
    return true;
  }
  else
    return false;
}

bool ConfigValue::GetInt(int* value) const {
  if (!value || value_.empty())
    return false;
  // try to parse hex number
  if (boost::starts_with(value_, "0x")) {
    char* p = NULL;
    unsigned int hex = std::strtoul(value_.c_str(), &p, 16);
    if (*p == '\0') {
      *value = static_cast<int>(hex);
      return true;
    }
  }
  // decimal
  try {
    *value = boost::lexical_cast<int>(value_);
  }
  catch (...) {
    return false;
  }
  return true;
}

bool ConfigValue::GetDouble(double* value) const {
  if (!value || value_.empty())
    return false;
  try {
    *value = boost::lexical_cast<double>(value_);
  }
  catch (...) {
    return false;
  }
  return true;
}

bool ConfigValue::GetString(string* value) const {
  if (!value) return false;
  *value = value_;
  return true;
}

bool ConfigValue::SetBool(bool value) {
  value_ = value ? "true" : "false";
  return true;
}

bool ConfigValue::SetInt(int value) {
  value_ = boost::lexical_cast<string>(value);
  return true;
}

bool ConfigValue::SetDouble(double value) {
  value_ = boost::lexical_cast<string>(value);
  return true;
}

bool ConfigValue::SetString(const char* value) {
  value_ = value;
  return true;
}

bool ConfigValue::SetString(const string& value) {
  value_ = value;
  return true;
}

// ConfigList members

an<ConfigItem> ConfigList::GetAt(size_t i) const {
  if (i >= seq_.size())
    return nullptr;
  else
    return seq_[i];
}

an<ConfigValue> ConfigList::GetValueAt(size_t i) const {
  return As<ConfigValue>(GetAt(i));
}

bool ConfigList::SetAt(size_t i, an<ConfigItem> element) {
  if (i >= seq_.size())
    seq_.resize(i + 1);
  seq_[i] = element;
  return true;
}

bool ConfigList::Insert(size_t i, an<ConfigItem> element) {
  if (i > seq_.size()) {
    seq_.resize(i);
  }
  seq_.insert(seq_.begin() + i, element);
  return true;
}

bool ConfigList::Append(an<ConfigItem> element) {
  seq_.push_back(element);
  return true;
}

bool ConfigList::Resize(size_t size) {
  seq_.resize(size);
  return true;
}

bool ConfigList::Clear() {
  seq_.clear();
  return true;
}

size_t ConfigList::size() const {
  return seq_.size();
}

ConfigList::Iterator ConfigList::begin() {
  return seq_.begin();
}

ConfigList::Iterator ConfigList::end() {
  return seq_.end();
}

// ConfigMap members

bool ConfigMap::HasKey(const string& key) const {
  return bool(Get(key));
}

an<ConfigItem> ConfigMap::Get(const string& key) const {
  auto it = map_.find(key);
  if (it == map_.end())
    return nullptr;
  else
    return it->second;
}

an<ConfigValue> ConfigMap::GetValue(const string& key) const {
  return As<ConfigValue>(Get(key));
}

bool ConfigMap::Set(const string& key, an<ConfigItem> element) {
  map_[key] = element;
  return true;
}

bool ConfigMap::Clear() {
  map_.clear();
  return true;
}

ConfigMap::Iterator ConfigMap::begin() {
  return map_.begin();
}

ConfigMap::Iterator ConfigMap::end() {
  return map_.end();
}

// ConfigItemRef members

bool ConfigItemRef::IsNull() const {
  auto item = GetItem();
  return !item || item->type() == ConfigItem::kNull;
}

bool ConfigItemRef::IsValue() const {
  auto item = GetItem();
  return item && item->type() == ConfigItem::kScalar;
}

bool ConfigItemRef::IsList() const {
  auto item = GetItem();
  return item && item->type() == ConfigItem::kList;
}

bool ConfigItemRef::IsMap() const {
  auto item = GetItem();
  return item && item->type() == ConfigItem::kMap;
}

bool ConfigItemRef::ToBool() const {
  bool value = false;
  if (auto item = As<ConfigValue>(GetItem())) {
    item->GetBool(&value);
  }
  return value;
}

int ConfigItemRef::ToInt() const {
  int value = 0;
  if (auto item = As<ConfigValue>(GetItem())) {
    item->GetInt(&value);
  }
  return value;
}

double ConfigItemRef::ToDouble() const {
  double value = 0.0;
  if (auto item = As<ConfigValue>(GetItem())) {
    item->GetDouble(&value);
  }
  return value;
}

string ConfigItemRef::ToString() const {
  string value;
  if (auto item = As<ConfigValue>(GetItem())) {
    item->GetString(&value);
  }
  return value;
}

an<ConfigList> ConfigItemRef::AsList() {
  auto list = As<ConfigList>(GetItem());
  if (!list)
    SetItem(list = New<ConfigList>());
  return list;
}

an<ConfigMap> ConfigItemRef::AsMap() {
  auto map = As<ConfigMap>(GetItem());
  if (!map)
    SetItem(map = New<ConfigMap>());
  return map;
}

void ConfigItemRef::Clear() {
  SetItem(nullptr);
}

bool ConfigItemRef::Append(an<ConfigItem> item) {
  if (AsList()->Append(item)) {
    set_modified();
    return true;
  }
  return false;
}

size_t ConfigItemRef::size() const {
  auto list = As<ConfigList>(GetItem());
  return list ? list->size() : 0;
}

bool ConfigItemRef::HasKey(const string& key) const {
  auto map = As<ConfigMap>(GetItem());
  return map ? map->HasKey(key) : false;
}

bool ConfigItemRef::modified() const {
  return data_ && data_->modified();
}

void ConfigItemRef::set_modified() {
  if (data_)
    data_->set_modified();
}

// Config members

Config::Config() : ConfigItemRef(New<ConfigData>()) {
}

Config::~Config() {
}

Config::Config(const string& file_name)
    : ConfigItemRef(ConfigDataManager::instance().GetConfigData(file_name)) {
}

bool Config::LoadFromStream(std::istream& stream) {
  return data_->LoadFromStream(stream);
}

bool Config::SaveToStream(std::ostream& stream) {
  return data_->SaveToStream(stream);
}

bool Config::LoadFromFile(const string& file_name) {
  return data_->LoadFromFile(file_name);
}

bool Config::SaveToFile(const string& file_name) {
  return data_->SaveToFile(file_name);
}

bool Config::IsNull(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kNull;
}

bool Config::IsValue(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kScalar;
}

bool Config::IsList(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kList;
}

bool Config::IsMap(const string& key) {
  auto p = data_->Traverse(key);
  return !p || p->type() == ConfigItem::kMap;
}

bool Config::GetBool(const string& key, bool* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetBool(value);
}

bool Config::GetInt(const string& key, int* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetInt(value);
}

bool Config::GetDouble(const string& key, double* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetDouble(value);
}

bool Config::GetString(const string& key, string* value) {
  DLOG(INFO) << "read: " << key;
  auto p = As<ConfigValue>(data_->Traverse(key));
  return p && p->GetString(value);
}

an<ConfigItem> Config::GetItem(const string& key) {
  DLOG(INFO) << "read: " << key;
  return data_->Traverse(key);
}

an<ConfigValue> Config::GetValue(const string& key) {
  DLOG(INFO) << "read: " << key;
  return As<ConfigValue>(data_->Traverse(key));
}

an<ConfigList> Config::GetList(const string& key) {
  DLOG(INFO) << "read: " << key;
  return As<ConfigList>(data_->Traverse(key));
}

an<ConfigMap> Config::GetMap(const string& key) {
  DLOG(INFO) << "read: " << key;
  return As<ConfigMap>(data_->Traverse(key));
}

bool Config::SetBool(const string& key, bool value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetInt(const string& key, int value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetDouble(const string& key, double value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetString(const string& key, const char* value) {
  return SetItem(key, New<ConfigValue>(value));
}

bool Config::SetString(const string& key, const string& value) {
  return SetItem(key, New<ConfigValue>(value));
}

static inline bool IsListItemReference(const string& key) {
  return !key.empty() && key[0] == '@';
}

static size_t ResolveListIndex(an<ConfigItem> p, const string& key,
                               bool read_only = false) {
  //if (!IsListItemReference(key)) {
  //  return 0;
  //}
  an<ConfigList> list = As<ConfigList>(p);
  if (!list) {
    return 0;
  }
  const string kAfter("after");
  const string kBefore("before");
  const string kLast("last");
  const string kNext("next");
  size_t cursor = 1;
  unsigned int index = 0;
  bool will_insert = false;
  if (key.compare(cursor, kNext.length(), kNext) == 0) {
    cursor += kNext.length();
    index = list->size();
  }
  else if (key.compare(cursor, kBefore.length(), kBefore) == 0) {
    cursor += kBefore.length();
    will_insert = true;
  }
  else if (key.compare(cursor, kAfter.length(), kAfter) == 0) {
    cursor += kAfter.length();
    index += 1;  // after i == before i+1
    will_insert = true;
  }
  if (cursor < key.length() && key[cursor] == ' ') {
    ++cursor;
  }
  if (key.compare(cursor, kLast.length(), kLast) == 0) {
    cursor += kLast.length();
    index += list->size();
    if (index != 0) {  // when list is empty, (before|after) last == 0
      --index;
    }
  }
  else {
    index += std::strtoul(key.c_str() + cursor, NULL, 10);
  }
  if (will_insert && !read_only) {
    list->Insert(index, nullptr);
  }
  return index;
}

bool Config::SetItem(const string& key, an<ConfigItem> item) {
  LOG(INFO) << "write: " << key;
  if (key.empty() || key == "/") {
    data_->root = item;
    data_->set_modified();
    return true;
  }
  if (!data_->root) {
    data_->root = New<ConfigMap>();
  }
  an<ConfigItem> p(data_->root);
  vector<string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  size_t k = keys.size() - 1;
  for (size_t i = 0; i <= k; ++i) {
    ConfigItem::ValueType node_type = ConfigItem::kMap;
    size_t list_index = 0;
    if (IsListItemReference(keys[i])) {
      node_type = ConfigItem::kList;
      list_index = ResolveListIndex(p, keys[i]);
      DLOG(INFO) << "list index " << keys[i] << " == " << list_index;
    }
    if (!p || p->type() != node_type) {
      return false;
    }
    if (i == k) {
      if (node_type == ConfigItem::kList) {
        As<ConfigList>(p)->SetAt(list_index, item);
      }
      else {
        As<ConfigMap>(p)->Set(keys[i], item);
      }
      data_->set_modified();
      return true;
    }
    else {
      an<ConfigItem> next;
      if (node_type == ConfigItem::kList) {
        next = As<ConfigList>(p)->GetAt(list_index);
      }
      else {
        next = As<ConfigMap>(p)->Get(keys[i]);
      }
      if (!next) {
        if (IsListItemReference(keys[i + 1])) {
          DLOG(INFO) << "creating list node for key: " << keys[i + 1];
          next = New<ConfigList>();
        }
        else {
          DLOG(INFO) << "creating map node for key: " << keys[i + 1];
          next = New<ConfigMap>();
        }
        if (node_type == ConfigItem::kList) {
          As<ConfigList>(p)->SetAt(list_index, next);
        }
        else {
          As<ConfigMap>(p)->Set(keys[i], next);
        }
      }
      p = next;
    }
  }
  return false;
}

an<ConfigItem> Config::GetItem() const {
  return data_->root;
}

void Config::SetItem(an<ConfigItem> item) {
  data_->root = item;
  set_modified();
}

// ConfigComponent members

string ConfigComponent::GetConfigFilePath(const string& config_id) {
  return boost::str(boost::format(pattern_) % config_id);
}

Config* ConfigComponent::Create(const string& config_id) {
  string path(GetConfigFilePath(config_id));
  DLOG(INFO) << "config file path: " << path;
  return new Config(path);
}

// ConfigDataManager memebers

ConfigDataManager& ConfigDataManager::instance() {
  static the<ConfigDataManager> s_instance;
  if (!s_instance) {
    s_instance.reset(new ConfigDataManager);
  }
  return *s_instance;
}

an<ConfigData>
ConfigDataManager::GetConfigData(const string& config_file_path) {
  an<ConfigData> sp;
  // keep a weak reference to the shared config data in the manager
  weak<ConfigData>& wp((*this)[config_file_path]);
  if (wp.expired()) {  // create a new copy and load it
    sp = New<ConfigData>();
    sp->LoadFromFile(config_file_path);
    wp = sp;
  }
  else {  // obtain the shared copy
    sp = wp.lock();
  }
  return sp;
}

bool ConfigDataManager::ReloadConfigData(const string& config_file_path) {
  iterator it = find(config_file_path);
  if (it == end()) {  // never loaded
    return false;
  }
  an<ConfigData> sp = it->second.lock();
  if (!sp)  {  // already been freed
    erase(it);
    return false;
  }
  sp->LoadFromFile(config_file_path);
  return true;
}

// ConfigData members

ConfigData::~ConfigData() {
  if (modified_ && !file_name_.empty())
    SaveToFile(file_name_);
}

bool ConfigData::LoadFromStream(std::istream& stream) {
  if (!stream.good()) {
    LOG(ERROR) << "failed to load config from stream.";
    return false;
  }
  try {
    YAML::Node doc = YAML::Load(stream);
    root = ConvertFromYaml(doc);
  }
  catch (YAML::Exception& e) {
    LOG(ERROR) << "Error parsing YAML: " << e.what();
    return false;
  }
  return true;
}

bool ConfigData::SaveToStream(std::ostream& stream) {
  if (!stream.good()) {
    LOG(ERROR) << "failed to save config to stream.";
    return false;
  }
  try {
    YAML::Emitter emitter(stream);
    EmitYaml(root, &emitter, 0);
  }
  catch (YAML::Exception& e) {
    LOG(ERROR) << "Error emitting YAML: " << e.what();
    return false;
  }
  return true;
}

bool ConfigData::LoadFromFile(const string& file_name) {
  // update status
  file_name_ = file_name;
  modified_ = false;
  root.reset();
  if (!boost::filesystem::exists(file_name)) {
    LOG(WARNING) << "nonexistent config file '" << file_name << "'.";
    return false;
  }
  LOG(INFO) << "loading config file '" << file_name << "'.";
  try {
    YAML::Node doc = YAML::LoadFile(file_name);
    root = ConvertFromYaml(doc);
  }
  catch (YAML::Exception& e) {
    LOG(ERROR) << "Error parsing YAML: " << e.what();
    return false;
  }
  return true;
}

bool ConfigData::SaveToFile(const string& file_name) {
  // update status
  file_name_ = file_name;
  modified_ = false;
  if (file_name.empty()) {
    // not really saving
    return false;
  }
  LOG(INFO) << "saving config file '" << file_name << "'.";
  // dump tree
  std::ofstream out(file_name.c_str());
  return SaveToStream(out);
}

an<ConfigItem> ConfigData::Traverse(const string& key) {
  DLOG(INFO) << "traverse: " << key;
  if (key.empty() || key == "/") {
    return root;
  }
  vector<string> keys;
  boost::split(keys, key, boost::is_any_of("/"));
  // find the YAML::Node, and wrap it!
  an<ConfigItem> p = root;
  for (auto it = keys.begin(), end = keys.end(); it != end; ++it) {
    ConfigItem::ValueType node_type = ConfigItem::kMap;
    size_t list_index = 0;
    if (IsListItemReference(*it)) {
      node_type = ConfigItem::kList;
      list_index = ResolveListIndex(p, *it, true);
    }
    if (!p || p->type() != node_type) {
      return nullptr;
    }
    if (node_type == ConfigItem::kList) {
      p = As<ConfigList>(p)->GetAt(list_index);
    }
    else {
      p = As<ConfigMap>(p)->Get(*it);
    }
  }
  return p;
}

an<ConfigItem> ConfigData::ConvertFromYaml(const YAML::Node& node) {
  if (YAML::NodeType::Null == node.Type()) {
    return nullptr;
  }
  if (YAML::NodeType::Scalar == node.Type()) {
    return New<ConfigValue>(node.as<string>());
  }
  if (YAML::NodeType::Sequence == node.Type()) {
    auto config_list = New<ConfigList>();
    for (auto it = node.begin(), end = node.end(); it != end; ++it) {
      config_list->Append(ConvertFromYaml(*it));
    }
    return config_list;
  }
  else if (YAML::NodeType::Map == node.Type()) {
    auto config_map = New<ConfigMap>();
    for (auto it = node.begin(), end = node.end(); it != end; ++it) {
      string key = it->first.as<string>();
      config_map->Set(key, ConvertFromYaml(it->second));
    }
    return config_map;
  }
  return nullptr;
}

void ConfigData::EmitScalar(const string& str_value,
                            YAML::Emitter* emitter) {
  if (str_value.find_first_of("\r\n") != string::npos) {
    *emitter << YAML::Literal;
  }
  else if (!boost::algorithm::all(str_value,
                             boost::algorithm::is_alnum() ||
                             boost::algorithm::is_any_of("_."))) {
    *emitter << YAML::DoubleQuoted;
  }
  *emitter << str_value;
}

void ConfigData::EmitYaml(an<ConfigItem> node,
                          YAML::Emitter* emitter,
                          int depth) {
  if (!node || !emitter) return;
  if (node->type() == ConfigItem::kScalar) {
    auto value = As<ConfigValue>(node);
    EmitScalar(value->str(), emitter);
  }
  else if (node->type() == ConfigItem::kList) {
    if (depth >= 3) {
      *emitter << YAML::Flow;
    }
    *emitter << YAML::BeginSeq;
    auto list = As<ConfigList>(node);
    for (auto it = list->begin(), end = list->end(); it != end; ++it) {
      EmitYaml(*it, emitter, depth + 1);
    }
    *emitter << YAML::EndSeq;
  }
  else if (node->type() == ConfigItem::kMap) {
    if (depth >= 3) {
      *emitter << YAML::Flow;
    }
    *emitter << YAML::BeginMap;
    auto map = As<ConfigMap>(node);
    for (auto it = map->begin(), end = map->end(); it != end; ++it) {
      if (!it->second || it->second->type() == ConfigItem::kNull)
        continue;
      *emitter << YAML::Key;
      EmitScalar(it->first, emitter);
      *emitter << YAML::Value;
      EmitYaml(it->second, emitter, depth + 1);
    }
    *emitter << YAML::EndMap;
  }
}

}  // namespace rime
