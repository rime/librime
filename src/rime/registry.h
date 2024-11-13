//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REGISTRY_H_
#define RIME_REGISTRY_H_

#include <rime_api.h>
#include <rime/common.h>

namespace rime {

class ComponentBase;

class Registry {
 public:
  using ComponentMap = map<string, ComponentBase*, std::less<>>;

  RIME_API ComponentBase* Find(string_view name);
  RIME_API void Register(string_view name, ComponentBase* component);
  RIME_API void Unregister(string_view name);
  void Clear();

  RIME_API static Registry& instance();

 private:
  Registry() = default;

  ComponentMap map_;
};

}  // namespace rime

#endif  // RIME_REGISTRY_H_
