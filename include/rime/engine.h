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

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

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
  boost::scoped_ptr<Schema> schema_;
  boost::scoped_ptr<Context> context_;
  std::vector<boost::shared_ptr<Processor> > processors_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
