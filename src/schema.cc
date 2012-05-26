// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-08 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/schema.h>

namespace rime {

Schema::Schema() : schema_id_(".default"),
                   page_size_(5) {
  config_.reset(Config::Require("config")->Create("default"));
  FetchUsefulConfigItems();
}

Schema::Schema(const std::string &schema_id) : schema_id_(schema_id),
                                               page_size_(5) {
  if (boost::starts_with(schema_id_, L".")) {
    config_.reset(Config::Require("config")->Create(schema_id.substr(1)));
  }
  else {
    config_.reset(Config::Require("schema_config")->Create(schema_id));
  }
  FetchUsefulConfigItems();
}

void Schema::FetchUsefulConfigItems() {
  if (!config_) {
    schema_name_ = schema_id_ + "?";
    return;
  }
  if (!config_->GetString("schema/name", &schema_name_)) {
    schema_name_ = schema_id_;
  }
  if (!config_->GetInt("menu/page_size", &page_size_) &&
      schema_id_ != ".default") {
    // not defined in schema, use default setting
    scoped_ptr<Config> default_config(
        Config::Require("config")->Create("default"));
    if (default_config)
      default_config->GetInt("menu/page_size", &page_size_);
  }
  config_->GetString("menu/alternative_select_keys", &select_keys_);
}

}  // namespace rime
