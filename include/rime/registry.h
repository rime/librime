//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REGISTRY_H_
#define RIME_REGISTRY_H_

#include <rime/common.h>

namespace rime {

class ComponentBase;

class Registry {
 public:
  using ComponentMap = map<string, ComponentBase*>;

  ComponentBase* Find(const string& name);
  void Register(const string& name, ComponentBase* component);
  void Unregister(const string& name);
  void Clear();

  static Registry& instance();

 private:
  Registry() = default;

  ComponentMap map_;
};

}  // namespace rime

#endif  // RIME_REGISTRY_H_
