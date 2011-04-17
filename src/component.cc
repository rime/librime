// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#include <map>
#include <string>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

class ComponentRegistry
    : public std::map<std::string, shared_ptr<Component> > {
 public:
  static ComponentRegistry& GetInstance() { return registry_; }

 private:
  ComponentRegistry() {}
  static ComponentRegistry registry_;
};

ComponentRegistry ComponentRegistry::registry_;

void Component::Register(const std::string &name, Component *component) {
  EZLOGGERPRINT("registering component: %s", name.c_str());
  ComponentRegistry &registry = ComponentRegistry::GetInstance();
  registry[name] = shared_ptr<Component>(component);
}

Component* Component::ByName(const std::string &name) {
  ComponentRegistry &registry = ComponentRegistry::GetInstance();
  ComponentRegistry::const_iterator it = registry.find(name);
  if (it != registry.end()) {
    return it->second.get();
  }
  return NULL;
}

}  // namespace rime
