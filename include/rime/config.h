//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIG_H_
#define RIME_CONFIG_H_

#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

// config item base class
class ConfigItem {
 public:
  enum ValueType { kNull, kScalar, kList, kMap };

  ConfigItem() = default;  // null
  virtual ~ConfigItem() = default;

  ValueType type() const { return type_; }

 protected:
  ConfigItem(ValueType type) : type_(type) {}

  ValueType type_ = kNull;
};

using ConfigItemPtr = shared_ptr<ConfigItem>;

class ConfigValue : public ConfigItem {
 public:
  ConfigValue() : ConfigItem(kScalar) {}
  ConfigValue(bool value);
  ConfigValue(int value);
  ConfigValue(double value);
  ConfigValue(const char* value);
  ConfigValue(const std::string& value);

  // schalar value accessors
  bool GetBool(bool* value) const;
  bool GetInt(int* value) const;
  bool GetDouble(double* value) const;
  bool GetString(std::string* value) const;
  bool SetBool(bool value);
  bool SetInt(int value);
  bool SetDouble(double value);
  bool SetString(const char* value);
  bool SetString(const std::string& value);

  const std::string& str() const { return value_; }

 protected:
  std::string value_;
};

using ConfigValuePtr = shared_ptr<ConfigValue>;

class ConfigList : public ConfigItem {
 public:
  using Sequence = std::vector<ConfigItemPtr>;
  using Iterator = Sequence::iterator;

  ConfigList() : ConfigItem(kList) {}
  ConfigItemPtr GetAt(size_t i) const;
  ConfigValuePtr GetValueAt(size_t i) const;
  bool SetAt(size_t i, ConfigItemPtr element);
  bool Insert(size_t i, ConfigItemPtr element);
  bool Append(ConfigItemPtr element);
  bool Resize(size_t size);
  bool Clear();
  size_t size() const;

  Iterator begin();
  Iterator end();

 protected:
  Sequence seq_;
};

using ConfigListPtr = shared_ptr<ConfigList>;

// limitation: map keys have to be strings, preferably alphanumeric
class ConfigMap : public ConfigItem {
 public:
  using Map = std::map<std::string, ConfigItemPtr>;
  using Iterator = Map::iterator;

  ConfigMap() : ConfigItem(kMap) {}
  bool HasKey(const std::string& key) const;
  ConfigItemPtr Get(const std::string& key) const;
  ConfigValuePtr GetValue(const std::string& key) const;
  bool Set(const std::string& key, ConfigItemPtr element);
  bool Clear();

  Iterator begin();
  Iterator end();

 protected:
  Map map_;
};

using ConfigMapPtr = shared_ptr<ConfigMap>;

class ConfigData;
class ConfigListEntryRef;
class ConfigMapEntryRef;

class ConfigItemRef {
 public:
  ConfigItemRef(const shared_ptr<ConfigData>& data) : data_(data) {
  }
  operator ConfigItemPtr () const {
    return GetItem();
  }
  ConfigListEntryRef operator[] (size_t index);
  ConfigMapEntryRef operator[] (const std::string& key);

  bool IsNull() const;
  bool IsValue() const;
  bool IsList() const;
  bool IsMap() const;

  bool ToBool() const;
  int ToInt() const;
  double ToDouble() const;
  std::string ToString() const;

  ConfigListPtr AsList();
  ConfigMapPtr AsMap();
  void Clear();

  // list
  bool Append(ConfigItemPtr item);
  size_t size() const;
  // map
  bool HasKey(const std::string& key) const;

  bool modified() const;
  void set_modified();

 protected:
  virtual ConfigItemPtr GetItem() const = 0;
  virtual void SetItem(ConfigItemPtr item) = 0;

  shared_ptr<ConfigData> data_;
};

namespace {

template <class T>
ConfigItemPtr AsConfigItem(const T& a, const std::false_type&) {
  return New<ConfigValue>(a);
};

template <class T>
ConfigItemPtr AsConfigItem(const T& a, const std::true_type&) {
  return a;
};

}  // namespace

class ConfigListEntryRef : public ConfigItemRef {
 public:
  ConfigListEntryRef(shared_ptr<ConfigData> data,
                     ConfigListPtr list, size_t index)
      : ConfigItemRef(data), list_(list), index_(index) {
  }
  template <class T>
  ConfigListEntryRef& operator= (const T& a) {
    SetItem(AsConfigItem(a, std::is_convertible<T, ConfigItemPtr>()));
    return *this;
  }
 protected:
  ConfigItemPtr GetItem() const {
    return list_->GetAt(index_);
  }
  void SetItem(ConfigItemPtr item) {
    list_->SetAt(index_, item);
    set_modified();
  }
 private:
  ConfigListPtr list_;
  size_t index_;
};

class ConfigMapEntryRef : public ConfigItemRef {
 public:
  ConfigMapEntryRef(shared_ptr<ConfigData> data,
                    ConfigMapPtr map, const std::string& key)
      : ConfigItemRef(data), map_(map), key_(key) {
  }
  template <class T>
  ConfigMapEntryRef& operator= (const T& a) {
    SetItem(AsConfigItem(a, std::is_convertible<T, ConfigItemPtr>()));
    return *this;
  }
 protected:
  ConfigItemPtr GetItem() const {
    return map_->Get(key_);
  }
  void SetItem(ConfigItemPtr item) {
    map_->Set(key_, item);
    set_modified();
  }
 private:
  ConfigMapPtr map_;
  std::string key_;
};

inline ConfigListEntryRef ConfigItemRef::operator[] (size_t index) {
  return ConfigListEntryRef(data_, AsList(), index);
}

inline ConfigMapEntryRef ConfigItemRef::operator[] (const std::string& key) {
  return ConfigMapEntryRef(data_, AsMap(), key);
}

// Config class

class Config : public Class<Config, const std::string&>, public ConfigItemRef {
 public:
  // CAVEAT: Config instances created without argument will NOT
  // be managed by ConfigComponent
  Config();
  virtual ~Config();
  // instances of Config with identical file_name share a copy of config data
  // that could be reloaded by ConfigComponent once notified changes to the file
  explicit Config(const std::string& file_name);

  bool LoadFromStream(std::istream& stream);
  bool SaveToStream(std::ostream& stream);
  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);

  // access a tree node of a particular type with "path/to/key"
  bool IsNull(const std::string& key);
  bool IsValue(const std::string& key);
  bool IsList(const std::string& key);
  bool IsMap(const std::string& key);
  bool GetBool(const std::string& key, bool* value);
  bool GetInt(const std::string& key, int* value);
  bool GetDouble(const std::string& key, double* value);
  bool GetString(const std::string& key, std::string* value);

  ConfigItemPtr GetItem(const std::string& key);
  ConfigValuePtr GetValue(const std::string& key);
  ConfigListPtr GetList(const std::string& key);
  ConfigMapPtr GetMap(const std::string& key);

  // setters
  bool SetBool(const std::string& key, bool value);
  bool SetInt(const std::string& key, int value);
  bool SetDouble(const std::string& key, double value);
  bool SetString(const std::string& key, const char* value);
  bool SetString(const std::string& key, const std::string& value);
  // setter for adding / replacing items to the tree
  bool SetItem(const std::string& key, ConfigItemPtr item);

  template <class T>
  Config& operator= (const T& a) {
    SetItem(AsConfigItem(a, std::is_convertible<T, ConfigItemPtr>()));
    return *this;
  }

 protected:
  ConfigItemPtr GetItem() const;
  void SetItem(ConfigItemPtr item);
 };

// ConfigComponent class

class ConfigComponent : public Config::Component {
 public:
  ConfigComponent(const std::string& pattern) : pattern_(pattern) {}
  Config* Create(const std::string& config_id);
  std::string GetConfigFilePath(const std::string& config_id);
  const std::string& pattern() const { return pattern_; }

 private:
  std::string pattern_;
};

}  // namespace rime

#endif  // RIME_CONFIG_H_
