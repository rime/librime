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
#include <rime/setup.h>

namespace rime {

#define Q(x) #x
RIME_API RIME_MODULE_LIST(kDefaultModules, "default" RIME_EXTRA_MODULES);
#undef Q
RIME_MODULE_LIST(kDeployerModules, "deployer");
RIME_MODULE_LIST(kLegacyModules, "legacy");

RIME_REGISTER_MODULE_GROUP(default, "core", "dict", "gears")
RIME_REGISTER_MODULE_GROUP(deployer, "core", "dict", "levers")

RIME_API void LoadModules(const char* module_names[]) {
  ModuleManager& mm(ModuleManager::instance());
  for (const char** m = module_names; *m; ++m) {
    if (RimeModule* module = mm.Find(*m)) {
      mm.LoadModule(module);
    }
  }
}

RIME_API void SetupLogging(const char* app_name, int min_log_level, const char* log_dir) {
#ifdef RIME_ENABLE_LOGGING
  FLAGS_minloglevel = min_log_level;
  if (log_dir) {
    FLAGS_log_dir = log_dir;
  }
  // Do not allow other users to read/write log files created by current process.
  FLAGS_logfile_mode = 0600;
  google::InitGoogleLogging(app_name);
#endif  // RIME_ENABLE_LOGGING
}

RIME_API void SetupLogging(const char* app_name) {
  SetupLogging(app_name, 0, NULL);
}

}  // namespace rime
