//
// Copyright RIME Developers
// Distributed under the BSD License
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
  using boost::filesystem::path;

  LOG(INFO) << "registering core components.";
  Registry& r = Registry::instance();

  path user_data_dir = Service::instance().deployer().user_data_dir;
  path config_path = user_data_dir / "%s.yaml";
  path schema_path = user_data_dir / "%s.schema.yaml";

  r.Register("config", new ConfigComponent(config_path.string()));
  r.Register("schema_config", new ConfigComponent(schema_path.string()));
}

static void rime_core_finalize() {
  // registered components have been automatically destroyed prior to this call
}

RIME_REGISTER_MODULE(core)
