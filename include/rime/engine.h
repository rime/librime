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

#include <string>
#include <vector>
#include <boost/signals.hpp>
#include <rime/common.h>

namespace rime {

class Schema;
class Context;
class KeyEvent;
class Processor;
class Segmentor;
class Dictionary;

class Engine {
 public:
  typedef boost::signal<void (const std::string &commit_text)> CommitSink;

  Engine();
  ~Engine();

  bool ProcessKeyEvent(const KeyEvent &key_event);

  void set_schema(Schema *schema);
  Schema* schema() const { return schema_.get(); }
  Context* context() const { return context_.get(); }
  CommitSink& sink() { return sink_; }

 private:
  void InitializeComponents();
  void OnInputChange(Context *ctx);
  void OnCommit(Context *ctx);

  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  std::vector<shared_ptr<Processor> > processors_;
  std::vector<shared_ptr<Segmentor> > segmentors_;
  std::vector<shared_ptr<Dictionary> > dictionaries_;
  CommitSink sink_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
