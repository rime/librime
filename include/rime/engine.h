//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ENGINE_H_
#define RIME_ENGINE_H_

#include <string>
#include <rime/common.h>
#include <rime/messenger.h>

namespace rime {

class KeyEvent;
class Schema;
class Context;

class Engine : public Messenger {
 public:
  typedef signal<void (const std::string& commit_text)> CommitSink;

  virtual ~Engine();
  virtual bool ProcessKeyEvent(const KeyEvent &key_event) = 0;
  virtual void Attach(Engine* engine) { attached_engine_ = engine; }
  virtual void ApplySchema(Schema* schema) {}
  virtual void CommitText(std::string text) { sink_(text); }

  Schema* schema() const { return schema_.get(); }
  Context* context() const { return context_.get(); }
  CommitSink& sink() { return sink_; }
  Engine* attached_engine() const { return attached_engine_; }

  static Engine* Create(Schema *schema = NULL);

 protected:
  Engine(Schema *schema);

  scoped_ptr<Schema> schema_;
  scoped_ptr<Context> context_;
  CommitSink sink_;
  Engine* attached_engine_;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
