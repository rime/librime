//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMPONENT_H_
#define RIME_COMPONENT_H_

#include <rime/registry.h>

namespace rime {

class ComponentBase {
 public:
  ComponentBase() = default;
  virtual ~ComponentBase() = default;
};

template <class T, class Arg>
struct Class {
  using Initializer = Arg;

  class Component : public ComponentBase {
   public:
    virtual T* Create(Initializer arg) = 0;
  };

  static Component* Require(const string& name) {
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
