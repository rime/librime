//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MODULE_H_
#define RIME_MODULE_H_

#include <map>
#include <string>
#include <vector>
#include <rime_api.h>
#include <rime/common.h>

namespace rime {

class ModuleManager {
 public:
  // module is supposed to be a pointer to static variable
  void Register(const std::string& name, RimeModule* module);
  RimeModule* Find(const std::string& name);

  void LoadModule(RimeModule* module);
  void UnloadModules();

  static ModuleManager& instance() {
    if (!instance_) instance_.reset(new ModuleManager);
    return *instance_;
  }

 private:
  ModuleManager() {}
  static scoped_ptr<ModuleManager> instance_;

  // module registry
  typedef std::map<std::string, RimeModule*> ModuleMap;
  ModuleMap map_;
  // list of loaded modules
  std::vector<RimeModule*> loaded_;
};

}  // namespace rime

#endif  // RIME_MODULE_H_
