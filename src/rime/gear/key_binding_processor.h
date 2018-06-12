//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_KEY_BINDING_PROCESSOR_H_
#define RIME_KEY_BINDING_PROCESSOR_H_

#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/key_event.h>
#include <rime/processor.h>

namespace rime {

template <class T>
class KeyBindingProcessor {
 public:
  typedef void Handler(Context* ctx);
  using HandlerPtr = void (T::*)(Context* ctx);
  struct ActionDef {
    const char* name;
    HandlerPtr action;
  };

  static const ActionDef kActionNoop;

  KeyBindingProcessor(ActionDef* action_definitions)
      : action_definitions_(action_definitions) {}
  ProcessResult ProcessKeyEvent(const KeyEvent& key_event, Context* ctx);
  bool Accept(const KeyEvent& key_event, Context* ctx);
  void Bind(KeyEvent key_event, HandlerPtr action);
  void LoadConfig(Config* config, const string& section);

 private:
  ActionDef* action_definitions_;

  using KeyBindingMap = map<KeyEvent, HandlerPtr>;
  KeyBindingMap key_bindings_;
};

}  // namespace rime

#include <rime/gear/key_binding_processor_impl.h>

#endif  // RIME_KEY_BINDING_PROCESSOR_H_
