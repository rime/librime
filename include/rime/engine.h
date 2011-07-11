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

class KeyEvent;
class Schema;
class Composition;
class Context;
class Processor;
class Segmentor;
class Translator;

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
  void Compose(Context *ctx);
  void CalculateSegmentation(Composition *comp);
  void TranslateSegments(Composition *comp);
  void OnInputChange(Context *ctx);
  void OnCommit(Context *ctx);

  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  std::vector<shared_ptr<Processor> > processors_;
  std::vector<shared_ptr<Segmentor> > segmentors_;
  std::vector<shared_ptr<Translator> > translators_;
  CommitSink sink_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
