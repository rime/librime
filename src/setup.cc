//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-10-02 GONG Chen <chen.sst@gmail.com>
//

#include <glog/logging.h>
#include <rime/module.h>

namespace rime {

const char* kDefaultModules[] = { "core", "dict", "gears", "levers", NULL };
const char* kDeployerModules[] = { "core", "levers", NULL };

void LoadModules(const char* module_names[]) {
  ModuleManager& mm(ModuleManager::instance());
  for (const char** m = module_names; *m; ++m) {
    if (RimeModule* module = mm.Find(*m)) {
      mm.LoadModule(module);
    }
  }
}

void SetupLogging(const char* app_name) {
  google::InitGoogleLogging(app_name);
}

}  // namespace rime
