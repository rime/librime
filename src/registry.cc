//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/component.h>
#include <rime/registry.h>

namespace rime {

void Registry::Register(const string &name, ComponentBase *component) {
  LOG(INFO) << "registering component: " << name;
  if (ComponentBase* existing = Find(name)) {
    LOG(WARNING) << "replacing previously registered component: " << name;
    delete existing;
  }
  map_[name] = component;
}

void Registry::Unregister(const string &name) {
  LOG(INFO) << "unregistering component: " << name;
  ComponentMap::iterator it = map_.find(name);
  if (it == map_.end())
    return;
  delete it->second;
  map_.erase(it);
}

void Registry::Clear() {
  ComponentMap::iterator it = map_.begin();
  while (it != map_.end()) {
    delete it->second;
    map_.erase(it++);
  }
}

ComponentBase* Registry::Find(const string &name) {
  ComponentMap::const_iterator it = map_.find(name);
  if (it != map_.end()) {
    return it->second;
  }
  return NULL;
}

Registry& Registry::instance() {
  static the<Registry> s_instance;
  if (!s_instance) {
    s_instance.reset(new Registry);
  }
  return *s_instance;
}

}  // namespace rime
