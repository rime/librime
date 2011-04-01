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
#include <map>
#include <string>

namespace rime {

class ComponentRegistry
    : public std::map<std::string, shared_ptr<ComponentClass> > {
 public:
  static ComponentRegistry& GetInstance() { return registry_; }

 private:
  ComponentRegistry() {}
  static ComponentRegistry registry_;
};

ComponentRegistry ComponentRegistry::registry_;

bool ComponentClass::Register() {
  ComponentRegistry &registry = ComponentRegistry::GetInstance();
  if (registry.find(name()) == registry.end()) {
    registry[name()] = shared_ptr<ComponentClass>(this);
    return true;
  }
  return false;
}

shared_ptr<Component> Component::Create(const std::string &klass_name, 
                                        Engine* engine) {
  ComponentRegistry &registry = ComponentRegistry::GetInstance();
  ComponentRegistry::const_iterator it = registry.find(klass_name);
  if (it != registry.end()) {
    return it->second->CreateInstance(engine);
  }
  return shared_ptr<Component>();
}

}  // namespace rime
