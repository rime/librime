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

class ConfigDataManager : public std::map<std::string, weak_ptr<ConfigData> > {
 public:
  shared_ptr<ConfigData> GetConfigData(const std::string &config_file_path);
  bool ReloadConfigData(const std::string &config_file_path);
};

// Config class

class Config : public Class<Config, const std::string&> {
 public:
  // CAVEAT: Config instances created without argument will NOT
  // be managed by ConfigComponent
  Config();
  virtual ~Config();
  // instances of Config with identical file_name share a copy of config data
  // that could be reloaded by ConfigComponent once notified changes to the file
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
  shared_ptr<ConfigData> data_;
 };

// ConfigComponent class

class ConfigComponent : public Config::Component {
 public:
  
  ConfigComponent(const std::string &pattern) : pattern_(pattern) {}
  Config* Create(const std::string &config_id);
  const std::string GetConfigFilePath(const std::string &config_id);
  const std::string& pattern() const { return pattern_; }

  static void set_shared_data_dir(const std::string &dir) { shared_data_dir_ = dir; }
  static void set_user_data_dir(const std::string &dir) { user_data_dir_ = dir; }
  static const std::string& shared_data_dir() { return shared_data_dir_; }
  static const std::string& user_data_dir() { return user_data_dir_; }
  static ConfigDataManager& config_data_manager() { return config_data_manager_; }

 private:
  std::string pattern_;
  
  static std::string shared_data_dir_;
  static std::string user_data_dir_;
  static ConfigDataManager config_data_manager_;
};

}  // namespace rime

#endif  // RIME_CONFIG_H_
