// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// register components
//
// 2011-04-13 GONG Chen <chen.sst@gmail.com>
//

// TODO: include implementations of built-in components
#include <rime/common.h>
#include "yaml_config.h"

namespace rime {

void RegisterComponents()
{
  EZLOGGER("register built-in components");
  Component::Register("config", new YamlConfigComponent("config_path"));
}

}  // namespace rime
