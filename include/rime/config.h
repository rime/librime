//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-4-6 Zou xu <zouivex@gmail.com>
//
#ifndef RIME_CONFIG_H_
#define RIME_CONFIG_H_

#include <map>
#include <string>
#include <boost/type_traits.hpp>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

// config item base class
class ConfigItem {
 public:
  enum ValueType { kNull, kScalar, kList, kMap };

  // construct a null item
  ConfigItem() : type_(kNull) {}
  virtual ~ConfigItem() {}

  ValueType type() const { return type_; }

 protected:
  ConfigItem(ValueType type) : type_(type) {}
  
  ValueType type_;
};

typedef shared_ptr<ConfigItem> ConfigItemPtr;

class ConfigValue : public ConfigItem {
 public:
  ConfigValue() : ConfigItem(kScalar) {}
  ConfigValue(bool value);
  ConfigValue(int value);
  ConfigValue(double value);
  ConfigValue(const char *value);
  ConfigValue(const std::string &value);
  
  // schalar value accessors
  bool GetBool(bool *value) const;
  bool GetInt(int *value) const;
  bool GetDouble(double *value) const;
  bool GetString(std::string *value) const;
  bool SetBool(bool value);
  bool SetInt(int value);
  bool SetDouble(double value);
  bool SetString(const char *value);
  bool SetString(const std::string &value);
  
  const std::string& str() const { return value_; }
  
 protected:
  std::string value_;
};

typedef shared_ptr<ConfigValue> ConfigValuePtr;

class ConfigList : public ConfigItem {
 public:
  typedef std::vector<ConfigItemPtr> Sequence;
  typedef Sequence::iterator Iterator;
  
  ConfigList() : ConfigItem(kList) {}
  ConfigItemPtr GetAt(size_t i) const;
  ConfigValuePtr GetValueAt(size_t i) const;
  bool SetAt(size_t i, const ConfigItemPtr &element);
  bool Append(const ConfigItemPtr &element);
  bool Resize(size_t size);
  bool Clear();
  size_t size() const;

  Iterator begin();
  Iterator end();

 protected:
  Sequence seq_;
};

typedef shared_ptr<ConfigList> ConfigListPtr;

// limitation: map keys have to be strings, preferably alphanumeric
class ConfigMap : public ConfigItem {
 public:
  typedef std::map<std::string, ConfigItemPtr> Map;
  typedef Map::iterator Iterator;
  
  ConfigMap() : ConfigItem(kMap) {}
  bool HasKey(const std::string &key) const;
  ConfigItemPtr Get(const std::string &key) const;
  ConfigValuePtr GetValue(const std::string &key) const;
  bool Set(const std::string &key, const ConfigItemPtr &element);
  bool Clear();

  Iterator begin();
  Iterator end();

 protected:
  Map map_;
};

typedef shared_ptr<ConfigMap> ConfigMapPtr;

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
  const std::string ToString() const;

  ConfigListPtr AsList();
  ConfigMapPtr AsMap();
  void Clear();
  
  // list
  bool Append(const ConfigItemPtr& item);
  size_t size() const;
  // map
  bool HasKey(const std::string& key) const;

  bool modified() const;
  void set_modified();

 protected:
  virtual ConfigItemPtr GetItem() const = 0;
  virtual void SetItem(const ConfigItemPtr& item) = 0;

  shared_ptr<ConfigData> data_;
};

namespace {

template <class T>
ConfigItemPtr AsConfigItem(const T& a, const boost::false_type&) {
  return New<ConfigValue>(a);
};

template <class T>
ConfigItemPtr AsConfigItem(const T& a, const boost::true_type&) {
  return a;
};

}  // namespace

class ConfigListEntryRef : public ConfigItemRef {
 public:
  ConfigListEntryRef(const shared_ptr<ConfigData>& data,
                     const ConfigListPtr& list, size_t index)
      : ConfigItemRef(data), list_(list), index_(index) {
  }
  template <class T>
  ConfigListEntryRef& operator= (const T& a) {
    SetItem(AsConfigItem(a, boost::is_convertible<T, ConfigItemPtr>()));
    return *this;
  }
 protected:
  ConfigItemPtr GetItem() const {
    return list_->GetAt(index_);
  }
  void SetItem(const ConfigItemPtr& item) {
    list_->SetAt(index_, item);
    set_modified();
  }
 private:
  ConfigListPtr list_;
  size_t index_;
};

class ConfigMapEntryRef : public ConfigItemRef {
 public:
  ConfigMapEntryRef(const shared_ptr<ConfigData>& data,
                    const ConfigMapPtr& map, const std::string& key)
      : ConfigItemRef(data), map_(map), key_(key) {
  }
  template <class T>
  ConfigMapEntryRef& operator= (const T& a) {
    SetItem(AsConfigItem(a, boost::is_convertible<T, ConfigItemPtr>()));
    return *this;
  }
 protected:
  ConfigItemPtr GetItem() const {
    return map_->Get(key_);
  }
  void SetItem(const ConfigItemPtr& item) {
    map_->Set(key_, item);
    set_modified();
  }
 private:
  ConfigMapPtr map_;
  const std::string key_;
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
  explicit Config(const std::string &file_name);

  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);

  // access a tree node of a particular type with "path/to/key"
  bool IsNull(const std::string &key);
  bool IsValue(const std::string &key);
  bool IsList(const std::string &key);
  bool IsMap(const std::string &key);
  bool GetBool(const std::string &key, bool *value);
  bool GetInt(const std::string &key, int *value);
  bool GetDouble(const std::string &key, double *value);
  bool GetString(const std::string &key, std::string *value);

  ConfigValuePtr GetValue(const std::string &key);
  ConfigListPtr GetList(const std::string &key);
  ConfigMapPtr GetMap(const std::string &key);

  // setters
  bool SetBool(const std::string &key, bool value);
  bool SetInt(const std::string &key, int value);
  bool SetDouble(const std::string &key, double value);
  bool SetString(const std::string &key, const char *value);
  bool SetString(const std::string &key, const std::string &value);
  // setter for adding / replacing items to the tree
  bool SetItem(const std::string &key, const ConfigItemPtr &item);

  template <class T>
  ConfigListEntryRef& operator= (const T& a) {
    SetItem(AsConfigItem(a, boost::is_convertible<T, ConfigItemPtr>()));
    return *this;
  }
  
 protected:
  ConfigItemPtr GetItem() const;
  void SetItem(const ConfigItemPtr& item);
 };

// ConfigComponent class

class ConfigComponent : public Config::Component {
 public:
  
  ConfigComponent(const std::string &pattern) : pattern_(pattern) {}
  Config* Create(const std::string &config_id);
  const std::string GetConfigFilePath(const std::string &config_id);
  const std::string& pattern() const { return pattern_; }

 private:
  std::string pattern_;
};

}  // namespace rime

#endif  // RIME_CONFIG_H_
