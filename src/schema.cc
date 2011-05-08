// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-08 GONG Chen <chen.sst@gmail.com>
//
#include <rime/schema.h>

namespace rime {

Schema::Schema() : schema_id_("default") {
  config_.reset(Config::Require("config")->Create("default"));
}

Schema::Schema(const std::string &schema_id) : schema_id_(schema_id) {
  config_.reset(Config::Require("schema_config")->Create(schema_id));
}

}  // namespace rime
