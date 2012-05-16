// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/component.h>
#include <rime/registry.h>

namespace rime {

scoped_ptr<Registry> Registry::instance_;

void Registry::Register(const std::string &name, ComponentBase *component) {
  EZLOGGERPRINT("registering component: %s", name.c_str());
  map_[name] = component;
}

void Registry::Unregister(const std::string &name) {
  EZLOGGERPRINT("unregistering component: %s", name.c_str());
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

ComponentBase* Registry::Find(const std::string &name) {
  ComponentMap::const_iterator it = map_.find(name);
  if (it != map_.end()) {
    return it->second;
  }
  return NULL;
}

}  // namespace rime
