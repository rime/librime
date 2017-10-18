//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-08 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/schema.h>

namespace rime {

Schema::Schema()
    : schema_id_(".default") {
  config_.reset(Config::Require("config")->Create("default"));
  FetchUsefulConfigItems();
}

Schema::Schema(const string& schema_id)
    : schema_id_(schema_id) {
  config_.reset(boost::starts_with(schema_id_, L".") ?
                Config::Require("config")->Create(schema_id.substr(1)) :
                Config::Require("schema")->Create(schema_id));
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
  config_->GetInt("menu/page_size", &page_size_);
  config_->GetString("menu/alternative_select_keys", &select_keys_);
}

Config* SchemaComponent::Create(const string& schema_id) {
  return config_component_->Create(schema_id + ".schema");
}

}  // namespace rime
