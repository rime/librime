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

#include <rime/common.h>
#include <string>

namespace rime {

class Engine;

class Component {
 public:
  Component() {}
  virtual ~Component() {}
  // returns new component instance of given type or an empty ptr on error
  static shared_ptr<Component> Create(const std::string &klass_name,
                                      Engine *engine);
};


class ComponentClass {
 public:
  bool Register();
  virtual shared_ptr<Component> CreateInstance(Engine *engine) = 0;
  virtual const std::string name() const = 0;
};

}  // namespace rime

#endif  // RIME_COMPONENT_H_
