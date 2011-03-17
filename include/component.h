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

namespace rime
{

class Engine;
class Component;

class ComponentKlass
{
 public:
  bool Register();
  virtual const std::string name() const = 0;

 protected:
  virtual boost::shared_ptr<Component> CreateInstance(Engine *engine) = 0;
};

class Component
{
 public:
  Component(ComponentKlass *klass) : klass_(klass) {}
  ComponentKlass* klass() const { return klass_; }
  static boost::shared_ptr<Component> Create(const std::string &klass_name);

 private:
  ComponentKlass* klass_;
};

}  // namespace rime

#endif  // RIME_COMPONENT_H_
