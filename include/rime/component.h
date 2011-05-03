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

#include <string>
#include <rime/common.h>

namespace rime {

/*
// sample usage:
//
// define an interface to a family of interchangeable objects
// a nested Component class will be generated
class Greeting : public Class_<Greeting, const std::string&> {
 public:
  virtual const std::string Salute() = 0;
};

// define an implementation of klass Greeting
class Hello : public Greeting {
 public:
  Hello(const std::string &name) {}
  // TODO: implement Salute()...
};

// the component implementation is simple and stupid
class HelloComponent : public Component_<Hello> {};

void SampleUsage() {
  // register a component
  Component::Register("my_greeting_way", new HelloComponent);

  // find a component for a known klass
  Greeting::Component *gc = Greeting::Find("my_greeting_way");

  // creating and using klass instances
  shared_ptr<Greeting> g1(gc->Create("fred"));
  cout << g1->Salute() << endl;
  shared_ptr<Greeting> g2(gc->Create("michael"));
  cout << g2->Salute() << endl;
}
*/

class Component {
 public:
  Component() {}
  virtual ~Component() {}
  // accessing the component registry
  static void Register(const std::string& name, Component *component);
  static void Unregister(const std::string& name);
  static void ClearRegistry();
  static Component* ByName(const std::string& name);
};

template <class K, class Arg>
struct Class_ {
  typedef Arg Initializer;

  class Component : public rime::Component {
   public:
    virtual K* Create(Initializer arg) = 0;
  };

  static Component* Find(const std::string& name) {
    return dynamic_cast<Component*>(Component::ByName(name));
  }
};

template <class T>
struct Component_ : public T::Component {
 public:
  T* Create(typename T::Initializer arg) {
    return new T(arg);
  }
};

// register components here!
void RegisterComponents(); 

}  // namespace rime

#endif  // RIME_COMPONENT_H_
