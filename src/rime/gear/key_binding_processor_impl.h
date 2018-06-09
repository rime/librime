//
// Copyright RIME Developers
// Distributed under the BSD License
//

namespace rime {

template <class T>
const typename KeyBindingProcessor<T>::ActionDef
    KeyBindingProcessor<T>::kActionNoop = { "noop", nullptr };

template <class T>
ProcessResult KeyBindingProcessor<T>::ProcessKeyEvent(
    const KeyEvent& key_event, Context* ctx) {
  // exact match
  if (Accept(key_event, ctx)) {
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
    if (Accept(shift_as_ctrl, ctx)) {
      return kAccepted;
    }
    KeyEvent ignore_shift{
      key_event.keycode(),
      key_event.modifier() & ~kShiftMask
    };
    if (Accept(ignore_shift, ctx)) {
      return kAccepted;
    }
  }
  // not handled
  return kNoop;
}

template <class T>
bool KeyBindingProcessor<T>::Accept(const KeyEvent& key_event, Context* ctx) {
  auto binding = key_bindings_.find(key_event);
  if (binding != key_bindings_.end()) {
    auto action = binding->second;
    RIME_THIS_CALL_AS(T, action)(ctx);
    DLOG(INFO) << "action key accepted: " << key_event.repr();
    return true;
  }
  return false;
}

template <class T>
void KeyBindingProcessor<T>::Bind(KeyEvent key_event, HandlerPtr action) {
  if (action) {
    key_bindings_[key_event] = action;
  }
  else {
    key_bindings_.erase(key_event);
  }
}

template <class T>
void KeyBindingProcessor<T>::LoadConfig(Config* config, const string& section) {
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
      Bind(ke, p->action);
    }
  }
}

}  // namespace rime
