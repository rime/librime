// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ENGINE_H_
#define RIME_ENGINE_H_

#include <rime/common.h>
#include <vector>

namespace rime {

class Schema;
class Context;
class Kevent;
class Processor;

class Engine {
 public:
  Engine();  // TODO(gongchen): arguments to argue
  ~Engine();
  bool ProcessKeyEvent(const Kevent &kevent);
  set_schema(Schema *schema);

 private:
  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  std::vector<shared_ptr<Processor> > processors_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
