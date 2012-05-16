// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REGISTRY_H_
#define RIME_REGISTRY_H_

#include <map>
#include <string>
#include <rime/common.h>

namespace rime {

class ComponentBase;

class Registry {
 public:
  typedef std::map<std::string, ComponentBase*> ComponentMap;

  ComponentBase* Find(const std::string& name);
  void Register(const std::string& name, ComponentBase *component);
  void Unregister(const std::string& name);
  void Clear();

  static Registry& instance() {
    if (!instance_) instance_.reset(new Registry);
    return *instance_;
  }

 private:
  Registry() {}
  static scoped_ptr<Registry> instance_;
  
  ComponentMap map_;
};

}  // namespace rime

#endif  // RIME_REGISTRY_H_
