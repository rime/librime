//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime/module.h>
#include <rime_api.h>

namespace rime {

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
  DLOG(INFO) << "loading module: " << module;
  if (!module)
    return;
  loaded_.push_back(module);
  if (module->initialize != NULL) {
    module->initialize();
  }
  else {
    LOG(WARNING) << "missing initialize() function in module: " << module;
  }
}

void ModuleManager::UnloadModules() {
  for (auto it = loaded_.cbegin(); it != loaded_.cend(); ++it) {
    if ((*it)->finalize != NULL) {
      (*it)->finalize();
    }
  }
  loaded_.clear();
}

ModuleManager& ModuleManager::instance() {
  static unique_ptr<ModuleManager> s_instance;
  if (!s_instance) {
    s_instance.reset(new ModuleManager);
  }
  return *s_instance;
}

}  // namespace rime
