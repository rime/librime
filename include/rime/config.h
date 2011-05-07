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

class ConfigItemData;

// config item base class
class ConfigItem {
 public:
  enum ValueType { kNull, kScalar, kList, kMap };

  // construct a null item
  ConfigItem()
      : type_(kNull), data_(NULL) {}
  ConfigItem(ValueType type)
      : type_(type), data_(NULL) {}
  ConfigItem(ValueType type, ConfigItemData *data)
      : type_(type), data_(data) {}
  virtual ~ConfigItem();

  // schalar value accessors 
  bool GetBool(bool *value) const;
  bool GetInt(int *value) const;
  bool GetDouble(double *value) const;
  bool GetString(std::string *value) const;
  void SetBool(bool value);
  void SetInt(int value);
  void SetDouble(double value);
  void SetString(const std::string &value);

  ValueType type() const { return type_; }

 protected:
  ValueType type_;
  ConfigItemData *data_;
};

typedef shared_ptr<ConfigItem> ConfigItemPtr;

class ConfigList : public ConfigItem {
 public:
  ConfigList() : ConfigItem(kList) {}
  ConfigList(ConfigItemData *data) : ConfigItem(kList, data) {}
  ConfigItemPtr GetAt(size_t i);
  void SetAt(size_t i, const ConfigItemPtr element);
  void Append(const ConfigItemPtr element);
  void Clear();
  size_t size() const;
};

// there is a limitation: keys have to be strings
class ConfigMap : public ConfigItem {
 public:
  ConfigMap() : ConfigItem(kMap) {}
  ConfigMap(ConfigItemData *data) : ConfigItem(kMap, data) {}
  bool HasKey(const std::string &key) const;
  ConfigItemPtr Get(const std::string &key);
  void Set(const std::string &key, const ConfigItemPtr element);
  void Clear();
};

class ConfigData;

// config component
class Config : public Class<Config, const std::string&> {
 public:
  Config();
  virtual ~Config();
  Config(const std::string &file_name);
 
  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);
 
  bool IsNull(const std::string &key);
  bool GetBool(const std::string &key, bool *value);
  bool GetInt(const std::string &key, int *value);
  bool GetDouble(const std::string &key, double *value);
  bool GetString(const std::string &key, std::string *value);
  shared_ptr<ConfigList> GetList(const std::string &key);
  shared_ptr<ConfigMap> GetMap(const std::string &key);

  // TODO: setters

 private:
  scoped_ptr<ConfigData> data_;
 };
 
class ConfigComponent : public Config::Component {
 public:
  ConfigComponent(const std::string &conf_dir) : conf_dir_(conf_dir) {}
  Config* Create(const std::string &file_name);
  const std::string& conf_dir() const { return conf_dir_; }

 private:
  std::string conf_dir_;
};

}  // namespace rime

#endif  // RIME_CONFIG_H_
