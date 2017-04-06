//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-06 Zou Xu <zouivex@gmail.com>
//
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

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

}  // namespace rime
