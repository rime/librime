//
// Copyright RIME Developers
// Distributed under the BSD License
//

namespace rime {

template <class T, int N>
const typename KeyBindingProcessor<T, N>::ActionDef
    KeyBindingProcessor<T, N>::kActionNoop = { "noop", nullptr };

template <class T, int N>
ProcessResult KeyBindingProcessor<T, N>::ProcessKeyEvent(
    const KeyEvent& key_event, Context* ctx, int keymap_selector) {
  auto& keymap = get_keymap(keymap_selector);
  // exact match
  if (Accept(key_event, ctx, keymap)) {
    return kAccepted;
  }
  // fallback: compatible modifiers
  if (key_event.ctrl() || key_event.alt()) {
    return kNoop;
  }
  if (key_event.shift()) {
    KeyEvent shift_as_ctrl{
      key_event.keycode(),
      (key_event.modifier() & ~kShiftMask) | kControlMask
    };
    if (Accept(shift_as_ctrl, ctx, keymap)) {
      return kAccepted;
    }
    KeyEvent ignore_shift{
      key_event.keycode(),
      key_event.modifier() & ~kShiftMask
    };
    if (Accept(ignore_shift, ctx, keymap)) {
      return kAccepted;
    }
  }
  // not handled
  return kNoop;
}

template <class T, int N>
typename KeyBindingProcessor<T, N>::Keymap&
KeyBindingProcessor<T, N>::get_keymap(int keymap_selector) {
  DCHECK_LT(keymap_selector, N);
  return keymaps_[keymap_selector];
}

template <class T, int N>
bool KeyBindingProcessor<T, N>::Accept(const KeyEvent& key_event,
                                       Context* ctx,
                                       Keymap& keymap) {
  auto binding = keymap.find(key_event);
  if (binding != keymap.end()) {
    auto action = binding->second;
    if (RIME_THIS_CALL_AS(T, action)(ctx)) {
      DLOG(INFO) << "action key accepted: " << key_event.repr();
      return true;
    }
  }
  return false;
}

template <class T, int N>
void KeyBindingProcessor<T, N>::Keymap::Bind(KeyEvent key_event,
                                             HandlerPtr action) {
  if (action) {
    (*this)[key_event] = action;
  }
  else {
    this->erase(key_event);
  }
}

template <class T, int N>
void KeyBindingProcessor<T, N>::LoadConfig(Config* config,
                                           const string& section,
                                           int keymap_selector) {
  auto& keymap = get_keymap(keymap_selector);
  if (auto bindings = config->GetMap(section + "/bindings")) {
    for (auto it = bindings->begin(); it != bindings->end(); ++it) {
      auto value = As<ConfigValue>(it->second);
      if (!value) {
        continue;
      }
      auto* p = action_definitions_;
      while (p->action && p->name != value->str()) {
        ++p;
      }
      if (!p->action && p->name != value->str()) {
        LOG(WARNING) << "[" << section << "] invalid action: " << value->str();
        continue;
      }
      KeyEvent ke;
      if (!ke.Parse(it->first)) {
        LOG(WARNING) << "[" << section << "] invalid key: " << it->first;
        continue;
      }
      keymap.Bind(ke, p->action);
    }
  }
}

}  // namespace rime
