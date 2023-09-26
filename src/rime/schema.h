//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SCHEMA_H_
#define RIME_SCHEMA_H_

#include <rime/common.h>
#include <rime/config.h>  // for convenience

namespace rime {

class Schema {
 public:
  Schema();
  explicit Schema(const string& schema_id);
  Schema(const string& schema_id, Config* config)
      : schema_id_(schema_id), config_(config) {}

  const string& schema_id() const { return schema_id_; }
  const string& schema_name() const { return schema_name_; }

  Config* config() const { return config_.get(); }
  void set_config(Config* config) { config_.reset(config); }

  int page_size() const { return page_size_; }
  bool page_down_cycle() const { return page_down_cycle_; }
  const string& select_keys() const { return select_keys_; }
  void set_select_keys(const string& keys) { select_keys_ = keys; }

 private:
  void FetchUsefulConfigItems();

  string schema_id_;
  string schema_name_;
  the<Config> config_;
  // frequently used config items
  int page_size_ = 5;
  bool page_down_cycle_ = false;
  string select_keys_;
};

class SchemaComponent : public Config::Component {
 public:
  SchemaComponent(Config::Component* config_component)
      : config_component_(config_component) {}
  // NOTE: creates `Config` for the schema
  Config* Create(const string& schema_id) override;

 private:
  // we do not own the config component, do not try to deallocate it
  // also be careful that there is no guarantee it will outlive us
  Config::Component* config_component_;
};

}  // namespace rime

#endif  // RIME_SCHEMA_H_
