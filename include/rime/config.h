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

#include <string>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {  

// config item base class
class ConfigItem {
 public:
  enum ValueType { kNull, kScalar, kList, kMap };

  ConfigItem() : type_(kNull) {}
  ConfigItem(ValueType type) : type_(type) {}

  // casting into specific node type
  template <class T>
  T* As() const { return dynamic_cast<T*>(this); }

  // schalar value accessors 
  template <class T>
  bool Get(T *value) const;
  template <class T>
  bool Set(T *value) const;

  ValueType type() const { return type_; }

 protected:
  ValueType type_;
};

// TODO: Null, Schalar type

class ConfigList : public ConfigItem {
 public:
  ConfigList() : ConfigItem(kList), length_(0) {}

  ConfigItem* GetAt(size_t n) const;
  // TODO: modifiers

  size_t length() const { return length_; }

 private:
  size_t length_;
};

class ConfigMap : public ConfigItem {
 public:
  ConfigMap() : ConfigItem(kMap) {}

  bool HasKey(const std::string &key) const;
  ConfigItem* Find(const std::string &key) const;
  // TODO: modifiers
};

// config component interface
class Config : public Class_<Config, std::string> {
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
