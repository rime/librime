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

template <class T, int N = 1>
class KeyBindingProcessor {
 public:
  typedef bool Handler(Context* ctx);
  using HandlerPtr = bool (T::*)(Context* ctx);

  enum FallbackOptions {
    None = 0,
    ShiftAsControl = (1 << 0),
    IgnoreShift = (1 << 1),
    All = ShiftAsControl | IgnoreShift,
  };

  struct ActionDef {
    const char* name;
    HandlerPtr action;
  };

  static const ActionDef kActionNoop;

  explicit KeyBindingProcessor(ActionDef* action_definitions)
      : action_definitions_(action_definitions) {}

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event,
                                Context* ctx,
                                int keymap_selector = 0,
                                int fallback_options = FallbackOptions::None);
  void LoadConfig(Config* config,
                  const string& section,
                  int kemap_selector = 0);

 protected:
  struct Keymap : map<KeyEvent, HandlerPtr> {
    void Bind(KeyEvent key_event, HandlerPtr action);
  };

  Keymap& get_keymap(int keymap_selector = 0);

  bool Accept(const KeyEvent& key_event, Context* ctx, Keymap& keymap);

 private:
  ActionDef* action_definitions_;
  Keymap keymaps_[N];
};

}  // namespace rime

#include <rime/gear/key_binding_processor_impl.h>

#endif  // RIME_KEY_BINDING_PROCESSOR_H_
