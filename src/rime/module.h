//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MODULE_H_
#define RIME_MODULE_H_

#include <unordered_set>
#include <rime/common.h>

struct rime_module_t;
typedef struct rime_module_t RimeModule;

namespace rime {

class ModuleManager {
 public:
  // module is supposed to be a pointer to static variable
  void Register(string_view name, RimeModule* module);
  RimeModule* Find(string_view name);

  void LoadModule(RimeModule* module);
  void UnloadModules();

  static ModuleManager& instance();

 private:
  ModuleManager() {}

  // module registry
  using ModuleMap = map<string, RimeModule*, std::less<>>;
  ModuleMap map_;
  // set of loaded modules
  std::unordered_set<RimeModule*> loaded_;
};

}  // namespace rime

#endif  // RIME_MODULE_H_
