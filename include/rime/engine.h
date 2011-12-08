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

#include <map>
#include <string>
#include <boost/signals.hpp>
#include <rime/common.h>

namespace rime {

class KeyEvent;
class Schema;
class Context;

class Engine {
 public:
  typedef boost::signal<void (const std::string &commit_text)> CommitSink;

  virtual ~Engine();
  virtual bool ProcessKeyEvent(const KeyEvent &key_event) = 0;
  virtual void set_schema(Schema *schema) {}

  Schema* schema() const { return schema_.get(); }
  Context* context() const { return context_.get(); }
  CommitSink& sink() { return sink_; }
  void set_option(const std::string &name, bool value);
  bool get_option(const std::string &name) const;

  static Engine* Create(Schema *schema = NULL);
  
 protected:
  Engine(Schema *schema);
  
  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  CommitSink sink_;
  std::map<std::string, bool> options_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
