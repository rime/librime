// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SCHEMA_H_
#define RIME_SCHEMA_H_

#include <rime/common.h>
#include <rime/config.h>

namespace rime {

// TODO:
class Schema {
 public:
  Schema() {}
  ~Schema() {}

  Config* config() const { return config_.get(); }
  void set_config(Config *config) { config_.reset(config); }

 private:
  scoped_ptr<Config> config_;
};

}  // namespace rime

#endif  // RIME_SCHEMA_H_
