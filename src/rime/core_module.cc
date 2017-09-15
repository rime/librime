//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>

// built-in components
#include <rime/config.h>
#include <rime/config/plugins.h>
#include <rime/schema.h>

using namespace rime;

static void rime_core_initialize() {
  LOG(INFO) << "registering core components.";
  Registry& r = Registry::instance();

  auto config = new ConfigComponent;
  config->InstallPlugin(new AutoPatchConfigPlugin);
  config->InstallPlugin(new LegacyPresetConfigPlugin);
  config->InstallPlugin(new LegacyDictionaryConfigPlugin);
  r.Register("config", config);
  r.Register("schema", new SchemaComponent(config));
}

static void rime_core_finalize() {
  // registered components have been automatically destroyed prior to this call
}

RIME_REGISTER_MODULE(core)
