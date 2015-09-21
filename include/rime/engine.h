//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ENGINE_H_
#define RIME_ENGINE_H_

#include <rime/common.h>
#include <rime/messenger.h>

namespace rime {

class KeyEvent;
class Schema;
class Context;

class Engine : public Messenger {
 public:
  using CommitSink = signal<void (const string& commit_text)>;

  virtual ~Engine();
  virtual bool ProcessKey(const KeyEvent& key_event) { return false; }
  virtual void ApplySchema(Schema* schema) {}
  virtual void CommitText(string text) { sink_(text); }
  virtual void Compose(Context* ctx) {}

  Schema* schema() const { return schema_.get(); }
  Context* context() const { return context_.get(); }
  CommitSink& sink() { return sink_; }

  Context* active_context() const {
    return active_context_ ? active_context_ : context_.get();
  }
  void set_active_context(Context* context = nullptr) {
    active_context_ = context;
  }

  static Engine* Create();

 protected:
  Engine();

  the<Schema> schema_;
  the<Context> context_;
  CommitSink sink_;
  Context* active_context_ = nullptr;
};

}  // namespace rime

#endif  // RIME_ENGINE_H_
