//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-02 GONG Chen <chen.sst@gmail.com>
//

#ifdef RIME_ENABLE_LOGGING
#include <glog/logging.h>
#endif  // RIME_ENABLE_LOGGING

#include <rime_api.h>
#include <rime/module.h>

namespace rime {

RIME_MODULE_LIST(kDefaultModules, "default");
RIME_MODULE_LIST(kDeployerModules, "deployer");
RIME_MODULE_LIST(kLegacyModules, "legacy");

void LoadModules(const char* module_names[]) {
  ModuleManager& mm(ModuleManager::instance());
  for (const char** m = module_names; *m; ++m) {
    if (RimeModule* module = mm.Find(*m)) {
      mm.LoadModule(module);
    }
  }
}

void SetupLogging(const char* app_name) {
#ifdef RIME_ENABLE_LOGGING
  google::InitGoogleLogging(app_name);
#endif  // RIME_ENABLE_LOGGING
}

RIME_REGISTER_MODULE_GROUP(default, "core", "dict", "gears")
RIME_REGISTER_MODULE_GROUP(deployer, "core", "dict", "levers")

}  // namespace rime
