//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime/module.h>
#include <rime_api.h>

namespace rime {

void ModuleManager::Register(const string& name,
                              RimeModule* module) {
  map_[name] = module;
}

RimeModule* ModuleManager::Find(const string& name) {
  ModuleMap::const_iterator it = map_.find(name);
  if (it != map_.end()) {
    return it->second;
  }
  return NULL;
}

void ModuleManager::LoadModule(RimeModule* module) {
  if (!module ||
      loaded_.find(module) != loaded_.end()) {
    return;
  }
  DLOG(INFO) << "loading module: " << module;
  loaded_.insert(module);
  if (module->initialize != NULL) {
    module->initialize();
  }
  else {
    LOG(WARNING) << "missing initialize() function in module: " << module;
  }
}

void ModuleManager::UnloadModules() {
  for (auto module : loaded_) {
    if (module->finalize != NULL) {
      module->finalize();
    }
  }
  loaded_.clear();
}

ModuleManager& ModuleManager::instance() {
  static the<ModuleManager> s_instance;
  if (!s_instance) {
    s_instance.reset(new ModuleManager);
  }
  return *s_instance;
}

}  // namespace rime
