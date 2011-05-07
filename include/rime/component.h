// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMPONENT_H_
#define RIME_COMPONENT_H_

#include <map>
#include <string>
#include <rime/registry.h>

namespace rime {

void RegisterRimeComponents(); 

class ComponentBase {
 public:
  ComponentBase() {}
  virtual ~ComponentBase() {}
};

template <class T, class Arg>
struct Class {
  typedef Arg Initializer;

  class Component : public ComponentBase {
   public:
    virtual T* Create(Initializer arg) = 0;
  };

  static Component* Require(const std::string& name) {
    return dynamic_cast<Component*>(Registry::instance().Find(name));
  }
};

template <class T>
struct Component : public T::Component {
 public:
  T* Create(typename T::Initializer arg) {
    return new T(arg);
  }
};

}  // namespace rime

#endif  // RIME_COMPONENT_H_
