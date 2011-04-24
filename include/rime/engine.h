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
#include <rime/common.h>

namespace rime {

class Schema;
class Context;
class KeyEvent;
class Processor;
class Dictionary;

class Engine {
 public:
  Engine();  // TODO(gongchen): arguments to argue
  ~Engine();

  bool ProcessKeyEvent(const KeyEvent &key_event);
  void set_schema(Schema *schema);

  Schema* schema() const { return schema_.get(); }
  Context* context() const { return context_.get(); }

 private:
  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  std::vector<shared_ptr<Processor> > processors_;
  std::vector<shared_ptr<Dictionary> > dictionaries_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
