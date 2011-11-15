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

struct ConfigItemData;

// config item base class
class ConfigItem {
 public:
  enum ValueType { kNull, kScalar, kList, kMap };

  // construct a null item
  ConfigItem();
  ConfigItem(ValueType type);
  ConfigItem(ValueType type, ConfigItemData *data);
  virtual ~ConfigItem();

  // schalar value accessors
  bool GetBool(bool *value) const;
  bool GetInt(int *value) const;
  bool GetDouble(double *value) const;
  bool GetString(std::string *value) const;
  bool SetBool(bool value);
  bool SetInt(int value);
  bool SetDouble(double value);
  bool SetString(const std::string &value);

  ValueType type() const { return type_; }
  ConfigItemData* data() const { return data_.get(); }

 protected:
  ValueType type_;
  scoped_ptr<ConfigItemData> data_;
};

typedef shared_ptr<ConfigItem> ConfigItemPtr;

class ConfigList : public ConfigItem {
 public:
  ConfigList() : ConfigItem(kList) {}
  ConfigList(ConfigItemData *data) : ConfigItem(kList, data) {}
  ConfigItemPtr GetAt(size_t i) const;
  bool SetAt(size_t i, const ConfigItemPtr element);
  bool Append(const ConfigItemPtr element);
  bool Clear();
  size_t size() const;
};

// limitation: map keys have to be strings, preferably alphanumeric
class ConfigMap : public ConfigItem {
 public:
  ConfigMap() : ConfigItem(kMap) {}
  ConfigMap(ConfigItemData *data) : ConfigItem(kMap, data) {}
  bool HasKey(const std::string &key) const;
  ConfigItemPtr Get(const std::string &key) const;
  bool Set(const std::string &key, const ConfigItemPtr element);
  bool Clear();
};

class ConfigData;

// ConfigDataManager class

class ConfigDataManager : public std::map<std::string, weak_ptr<ConfigData> > {
 public:
  shared_ptr<ConfigData> GetConfigData(const std::string &config_file_path);
  bool ReloadConfigData(const std::string &config_file_path);
  
  void set_shared_data_dir(const std::string &dir) { shared_data_dir_ = dir; }
  void set_user_data_dir(const std::string &dir) { user_data_dir_ = dir; }
  const std::string& shared_data_dir() { return shared_data_dir_; }
  const std::string& user_data_dir() { return user_data_dir_; }

  static ConfigDataManager& instance() {
    if (!instance_) instance_.reset(new ConfigDataManager);
    return *instance_;
  }

 private:
  ConfigDataManager() : shared_data_dir_("."), user_data_dir_(".") {}
  
  std::string shared_data_dir_;
  std::string user_data_dir_;

  static scoped_ptr<ConfigDataManager> instance_;
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
  explicit Config(const std::string &file_name);

  bool LoadFromFile(const std::string& file_name);
  bool SaveToFile(const std::string& file_name);

  // access a tree node of a particular type with "path/to/key"
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

 private:
  std::string pattern_;
};

}  // namespace rime

#endif  // RIME_CONFIG_H_
