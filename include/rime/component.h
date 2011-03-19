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
#include <boost/shared_ptr.hpp>

namespace rime {

class Engine;
class Component;

class ComponentClass {
 public:
  bool Register();
  virtual boost::shared_ptr<Component> CreateInstance(Engine *engine) = 0;
  virtual const std::string name() const = 0;
};

class Component {
 public:
  Component(ComponentClass *klass) : klass_(klass) {}
  ComponentClass* klass() const { return klass_; }

  // returns new component of specified type or an empty ptr on error
  static boost::shared_ptr<Component> Create(const std::string &klass_name, 
                                             Engine *engine);

 private:
  ComponentClass* klass_;
};

}  // namespace rime

#endif  // RIME_COMPONENT_H_
