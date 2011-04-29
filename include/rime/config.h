// vim: set sts=2 sw=2 et:
// encoding: utf-8
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
#include <vector>
#include <boost/any.hpp>
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

  // casting into container node
  template <class T>
  T* As() { return dynamic_cast<T*>(this); }

  // schalar value accessors 
  template <class T>
  bool get(T *value) const {
    if (type() != kScalar)
      return false;
    const T *cast_value = boost::any_cast<T>(&value_);
    if (cast_value) {
      *value = *cast_value;
      return true;
    }
    return false;
  }
  template <class T>
  void set(const T &value) { value_ = value; }
  // set value to a raw string
  void set(const char *value) { value_ = std::string(value); }

  ValueType type() const { return type_; }

 protected:
  ConfigItem(ValueType type) : type_(type) {}
  template <class T>
  ConfigItem(ValueType type, const T &value) : type_(type), value_(value) {}

  ValueType type_;
  boost::any value_;
};

class ConfigValue : public ConfigItem {
 public:
  // construct scalar with ... empty value
  ConfigValue() : ConfigItem(kScalar) {}
  // construct scalar with a given value
  template <class T>
  explicit ConfigValue(const T &value) : ConfigItem(kScalar, value) {}
  // construct scalar from a raw string 
  ConfigValue(const char *value) : ConfigItem(kScalar, std::string(value)) {}
  // creator that returns config item handle
  template <class T>
  static const shared_ptr<ConfigItem> Create(const T& value) {
    return shared_ptr<ConfigItem>(new ConfigValue(value));
  }
};

class ConfigList : public ConfigItem,
                   public std::vector<shared_ptr<ConfigItem> > {
 public:
  ConfigList() : ConfigItem(kList) {}
  static const shared_ptr<ConfigItem> Create() {
    return shared_ptr<ConfigItem>(new ConfigList());
  }
};

// there is a limitation: keys have to be strings
class ConfigMap : public ConfigItem,
                  public std::map<std::string, shared_ptr<ConfigItem> > {
 public:
  ConfigMap() : ConfigItem(kMap) {}
  static const shared_ptr<ConfigItem> Create() {
    return shared_ptr<ConfigItem>(new ConfigMap());
  }
};

// config component interface
class Config : public Class_<Config, const std::string&> {
 public:
  virtual bool IsNull(const std::string &key) = 0;
  virtual bool GetBool(const std::string &key, bool *value) = 0;
  virtual bool GetInt(const std::string &key, int *value) = 0;
  virtual bool GetDouble(const std::string &key, double *value) = 0;
  virtual bool GetString(const std::string &key, std::string *value) = 0;
  virtual ConfigList* GetList(const std::string &key) = 0;
  virtual ConfigMap* GetMap(const std::string &key) = 0;
  // TODO: setters
};

}  // namespace rime

#endif
