//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <rime/config/config_compiler.h>
#include <rime/config/config_cow_ref.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

ConfigData::~ConfigData() {
  if (auto_save_ && modified_ && !file_name_.empty())
    SaveToFile(file_name_);
}

bool ConfigData::LoadFromStream(std::istream& stream) {
  if (!stream.good()) {
    LOG(ERROR) << "failed to load config from stream.";
    return false;
  }
  try {
    YAML::Node doc = YAML::Load(stream);
    root = ConvertFromYaml(doc, nullptr);
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

bool ConfigData::LoadFromFile(const string& file_name,
                              ConfigCompiler* compiler) {
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
    root = ConvertFromYaml(doc, compiler);
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

bool ConfigData::IsListItemReference(const string& key) {
  return key.length() > 1 && key[0] == '@' && std::isalnum(key[1]);
}

string ConfigData::FormatListIndex(size_t index) {
  return boost::str(boost::format("@%u") % index);
}

static const string kAfter("after");
static const string kBefore("before");
static const string kLast("last");
static const string kNext("next");

size_t ConfigData::ResolveListIndex(an<ConfigItem> item, const string& key,
                                    bool read_only) {
  if (!IsListItemReference(key)) {
    return 0;
  }
  an<ConfigList> list = As<ConfigList>(item);
  if (!list) {
    return 0;
  }
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

class ConfigDataRootRef : public ConfigItemRef {
 public:
  ConfigDataRootRef(ConfigData* data) : ConfigItemRef(nullptr), data_(data) {
  }
  an<ConfigItem> GetItem() const override {
    return data_->root;
  }
  void SetItem(an<ConfigItem> item) override {
    data_->root = item;
  }
 private:
  ConfigData* data_;
};

an<ConfigItemRef> TypeCheckedCopyOnWrite(an<ConfigItemRef> parent,
                                         const string& key) {
  // special case to allow editing current node by __append: __merge: /+: /=:
  if (key.empty()) {
    return parent;
  }
  bool is_list = ConfigData::IsListItemReference(key);
  auto expected_node_type = is_list ? ConfigItem::kList : ConfigItem::kMap;
  an<ConfigItem> existing_node = *parent;
  if (existing_node && existing_node->type() != expected_node_type) {
    LOG(ERROR) << "copy on write failed; incompatible node type: " << key;
    return nullptr;
  }
  return Cow(parent, key);
}

an<ConfigItemRef> TraverseCopyOnWrite(an<ConfigItemRef> head,
                                      const string& path) {
  DLOG(INFO) << "TraverseCopyOnWrite(" << path << ")";
  if (path.empty() || path == "/") {
    return head;
  }
  vector<string> keys = ConfigData::SplitPath(path);
  size_t n = keys.size();
  for (size_t i = 0; i < n; ++i) {
    const auto& key = keys[i];
    if (auto child = TypeCheckedCopyOnWrite(head, key)) {
      head = child;
    } else {
      LOG(ERROR) << "while writing to " << path;
      return nullptr;
    }
  }
  return head;
}

bool ConfigData::TraverseWrite(const string& path, an<ConfigItem> item) {
  LOG(INFO) << "write: " << path;
  auto root = New<ConfigDataRootRef>(this);
  if (auto target = TraverseCopyOnWrite(root, path)) {
    *target = item;
    set_modified();
    return true;
  } else {
    return false;
  }
}

vector<string> ConfigData::SplitPath(const string& path) {
  vector<string> keys;
  auto is_separator = boost::is_any_of("/");
  auto trimmed_path = boost::trim_left_copy_if(path, is_separator);
  boost::split(keys, trimmed_path, is_separator);
  return keys;
}

string ConfigData::JoinPath(const vector<string>& keys) {
  return boost::join(keys, "/");
}

an<ConfigItem> ConfigData::Traverse(const string& path) {
  DLOG(INFO) << "traverse: " << path;
  if (path.empty() || path == "/") {
    return root;
  }
  vector<string> keys = SplitPath(path);
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

an<ConfigItem> ConfigData::ConvertFromYaml(
    const YAML::Node& node, ConfigCompiler* compiler) {
  if (YAML::NodeType::Null == node.Type()) {
    return nullptr;
  }
  if (YAML::NodeType::Scalar == node.Type()) {
    return New<ConfigValue>(node.as<string>());
  }
  if (YAML::NodeType::Sequence == node.Type()) {
    auto config_list = New<ConfigList>();
    for (auto it = node.begin(), end = node.end(); it != end; ++it) {
      if (compiler) {
        compiler->Push(config_list, config_list->size());
      }
      config_list->Append(ConvertFromYaml(*it, compiler));
      if (compiler) {
        compiler->Pop();
      }
    }
    return config_list;
  }
  else if (YAML::NodeType::Map == node.Type()) {
    auto config_map = New<ConfigMap>();
    for (auto it = node.begin(), end = node.end(); it != end; ++it) {
      string key = it->first.as<string>();
      if (compiler) {
        compiler->Push(config_map, key);
      }
      auto value = ConvertFromYaml(it->second, compiler);
      if (compiler) {
        compiler->Pop();
      }
      if (!compiler || !compiler->Parse(key, value)) {
        config_map->Set(key, value);
      }
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
