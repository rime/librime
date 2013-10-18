//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime/module.h>

namespace rime {

scoped_ptr<ModuleManager> ModuleManager::instance_;

void ModuleManager::Register(const std::string& name,
                              RimeModule* module) {
  map_[name] = module;
}

RimeModule* ModuleManager::Find(const std::string& name) {
  ModuleMap::const_iterator it = map_.find(name);
  if (it != map_.end()) {
    return it->second;
  }
  return NULL;
}

void ModuleManager::LoadModule(RimeModule* module) {
  if (!module) return;
  loaded_.push_back(module);
  if (module->initialize != NULL) {
    module->initialize();
  }
}

void ModuleManager::UnloadModules() {
  for (std::vector<RimeModule*>::const_iterator it = loaded_.begin();
       it != loaded_.end(); ++it) {
    if ((*it)->finalize != NULL) {
      (*it)->finalize();
    }
  }
  loaded_.clear();
}

}  // namespace rime
