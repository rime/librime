//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <boost/filesystem.hpp>
#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>
#include <rime/service.h>

// built-in components
#include <rime/config.h>

static void rime_core_initialize() {
  using namespace rime;

  LOG(INFO) << "registering core components";
  Registry &r = Registry::instance();

  boost::filesystem::path user_data_dir =
      Service::instance().deployer().user_data_dir;
  boost::filesystem::path config_path = user_data_dir / "%s.yaml";
  boost::filesystem::path schema_path = user_data_dir / "%s.schema.yaml";
  r.Register("config", new ConfigComponent(config_path.string()));
  r.Register("schema_config", new ConfigComponent(schema_path.string()));
}

static void rime_core_finalize() {
  // registered components have been automatically destroyed prior to this call
}

RimeModule* rime_core_module_init() {
  static RimeModule s_module = {0};
  if (!s_module.data_size) {
    RIME_STRUCT_INIT(RimeModule, s_module);
    s_module.initialize = rime_core_initialize;
    s_module.finalize = rime_core_finalize;
  }
  return &s_module;
}
