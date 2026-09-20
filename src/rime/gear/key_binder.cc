//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-23 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <cctype>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/switches.h>
#include <rime/gear/key_binder.h>

namespace rime {

enum KeyBindingCondition {
  kNever,
  kWhenPredicting,  // showing prediction candidates
  kWhenPaging,      // user has changed page
  kWhenHasMenu,     // at least one candidate
  kWhenComposing,   // input string is not empty
  kAlways,
};

static struct KeyBindingConditionDef {
  KeyBindingCondition condition;
  const char* name;
} condition_definitions[] = {{kWhenPredicting, "predicting"},
                             {kWhenPaging, "paging"},
                             {kWhenHasMenu, "has_menu"},
                             {kWhenComposing, "composing"},
                             {kAlways, "always"},
                             {kNever, NULL}};

static KeyBindingCondition translate_condition(const string& str) {
  for (auto* d = condition_definitions; d->name; ++d) {
    if (str == d->name)
      return d->condition;
  }
  return kNever;
}

struct KeyBinding {
  KeyBindingCondition whence;
  KeySequence target;
  function<void(Engine* engine)> action;

  bool operator<(const KeyBinding& o) const { return whence < o.whence; }
};

class KeyBindings : public map<KeyEvent, vector<KeyBinding>> {
 public:
  void LoadBindings(const an<ConfigList>& bindings);
  void Bind(const KeyEvent& key, const KeyBinding& binding);
};

static void radio_select_option(Context* ctx,
                                const Switches::SwitchOption& the_option) {
  Switches::FindRadioGroupOption(
      the_option.the_switch, [ctx, &the_option](Switches::SwitchOption option) {
        bool value = (option.option_index == the_option.option_index);
        if (ctx->get_option(option.option_name) != value) {
          ctx->set_option(option.option_name, value);
        }
        return Switches::kContinue;
      });
}

inline static bool is_switch_index(const string& option) {
  return !option.empty() && option.front() == '@';
}

static Switches::SwitchOption switch_by_index(Switches& switches,
                                              const string& option) {
  try {
    size_t index = std::stoul(option.substr(1));
    return switches.ByIndex(index);
  } catch (...) {
  }
  return {};
}

static void toggle_option(Engine* engine, const string& option) {
  if (!engine)
    return;
  Context* ctx = engine->context();
  Switches switches(engine->schema()->config());
  auto the_option = is_switch_index(option) ? switch_by_index(switches, option)
                                            : switches.OptionByName(option);
  if (the_option.found() && the_option.type == Switches::kRadioGroup) {
    auto selected_option = switches.FindRadioGroupOption(
        the_option.the_switch, [ctx](Switches::SwitchOption option) {
          return ctx->get_option(option.option_name) ? Switches::kFound
                                                     : Switches::kContinue;
        });
    if (!selected_option.found()) {
      // invalid state: none is selected. select the given option.
      radio_select_option(ctx, the_option);
      return;
    }
    // cycle through the ratio group and select the next option.
    auto next_option = Switches::Cycle(selected_option);
    if (next_option.found()) {
      radio_select_option(ctx, next_option);
    }
  } else {  // toggle
    // option can be an index. use the found option name, or an arbitrary
    // option name specified by caller.
    auto option_name = the_option.found() ? the_option.option_name : option;
    ctx->set_option(option_name, !ctx->get_option(option_name));
  }
}

static void set_option(Engine* engine, const string& option) {
  if (!engine)
    return;
  Context* ctx = engine->context();
  Switches switches(engine->schema()->config());
  auto the_option = switches.OptionByName(option);
  if (the_option.found() && the_option.type == Switches::kRadioGroup) {
    radio_select_option(ctx, the_option);
  } else {
    ctx->set_option(option, 1);
  }
}

static void unset_option(Engine* engine, const string& option) {
  if (!engine)
    return;
  Context* ctx = engine->context();
  Switches switches(engine->schema()->config());
  auto the_option = switches.OptionByName(option);
  if (the_option.found() && the_option.type == Switches::kRadioGroup) {
    if (ctx->get_option(option)) {
      auto default_option = Switches::Reset(the_option);
      if (default_option.found()) {
        radio_select_option(ctx, default_option);
      }
    }
  } else {
    ctx->set_option(option, 0);
  }
}

static void select_schema(Engine* engine, const string& schema) {
  if (!engine)
    return;
  if (schema == ".next") {
    Switcher switcher(engine);
    switcher.SelectNextSchema();
  } else {
    engine->ApplySchema(new Schema(schema));
  }
}

void KeyBindings::LoadBindings(const an<ConfigList>& bindings) {
  if (!bindings)
    return;
  for (size_t i = 0; i < bindings->size(); ++i) {
    auto map = As<ConfigMap>(bindings->GetAt(i));
    if (!map)
      continue;
    auto whence = map->GetValue("when");
    if (!whence)
      continue;
    auto pattern = map->GetValue("accept");
    if (!pattern)
      continue;
    KeyBinding binding;
    binding.whence = translate_condition(whence->str());
    if (binding.whence == kNever) {
      continue;
    }
    KeyEvent key;
    if (!key.Parse(pattern->str())) {
      LOG(WARNING) << "invalid key binding #" << i
                   << ", with invalid accept pattern: " << pattern->str()
                   << ".";
      continue;
    }
    if (auto target = map->GetValue("send")) {
      KeyEvent key;
      if (key.Parse(target->str())) {
        binding.target.push_back(std::move(key));
      } else {
        LOG(WARNING) << "invalid key binding #" << i
                     << ", with invalid send pattern: " << target->str() << ".";
        continue;
      }
    } else if (auto target = map->GetValue("send_sequence")) {
      if (!binding.target.Parse(target->str())) {
        LOG(WARNING) << "invalid key sequence #" << i
                     << ", with invalid send_sequence pattern: "
                     << target->str() << ".";
        continue;
      }
    } else if (auto option = map->GetValue("toggle")) {
      binding.action = [option](auto engine) {
        toggle_option(engine, option->str());
      };
    } else if (auto option = map->GetValue("set_option")) {
      binding.action = [option](auto engine) {
        set_option(engine, option->str());
      };
    } else if (auto option = map->GetValue("unset_option")) {
      binding.action = [option](auto engine) {
        unset_option(engine, option->str());
      };
    } else if (auto schema = map->GetValue("select")) {
      binding.action = [schema](auto engine) {
        select_schema(engine, schema->str());
      };
    } else {
      LOG(WARNING) << "invalid key binding #" << i
                   << ", accept: " << pattern->str()
                   << ", when: " << whence->str() << ".";
      continue;
    }
    Bind(key, binding);
  }
}

void KeyBindings::Bind(const KeyEvent& key, const KeyBinding& binding) {
  auto& vec = (*this)[key];
  // insert before existing binding of the same condition
  auto lb = std::lower_bound(vec.begin(), vec.end(), binding);
  vec.insert(lb, binding);
}

KeyBinder::KeyBinder(const Ticket& ticket)
    : Processor(ticket), key_bindings_(new KeyBindings) {
  LoadConfig();
}

class KeyBindingConditions : public set<KeyBindingCondition> {
 public:
  explicit KeyBindingConditions(Context* ctx);
};

KeyBindingConditions::KeyBindingConditions(Context* ctx) {
  insert(kAlways);

  if (ctx->IsComposing()) {
    insert(kWhenComposing);
  }

  if (ctx->HasMenu() && !ctx->get_option("ascii_mode")) {
    insert(kWhenHasMenu);
  }

  Composition& comp = ctx->composition();
  if (!comp.empty()) {
    const Segment& last_seg = comp.back();
    if (last_seg.HasTag("paging")) {
      insert(kWhenPaging);
    }
    if (last_seg.HasTag("prediction")) {
      insert(kWhenPredicting);
    }
  }
}

ProcessResult KeyBinder::ProcessKeyEvent(const KeyEvent& key_event) {
  if (redirecting_ || !key_bindings_ || key_bindings_->empty())
    return kNoop;

  // 1. 若上一鍵確實觸發了翻頁，且當前鍵是字母，則執行補償重釋
  if (ReinterpretPagingKey(key_event))
    return kNoop;

  if (key_bindings_->find(key_event) == key_bindings_->end()) {
    // 沒命中任何綁定，此鍵不是翻頁鍵，清除記錄
    if (!key_event.release()) {
      last_paging_key_ = 0;
      paging_keystroke_count_ = 0;
    }
    return kNoop;
  }

  KeyBindingConditions conditions(engine_->context());
  for (const KeyBinding& binding : (*key_bindings_)[key_event]) {
    if (conditions.find(binding.whence) == conditions.end())
      continue;

    // 1. 運行時檢查是否真正觸發了翻頁（不論上翻還是下翻）
    bool is_paging = std::any_of(
        binding.target.begin(), binding.target.end(), [](const KeyEvent& k) {
          return k.keycode() == XK_Page_Down ||
                 k.keycode() == XK_KP_Page_Down || k.keycode() == XK_Page_Up ||
                 k.keycode() == XK_KP_Page_Up;
        });

    // 2. 只有運行時真正派發了翻頁動作，才記錄該鍵等待後續字母驗證重釋
    if (is_paging) {
      last_paging_key_ = key_event.keycode();
      paging_keystroke_count_++;
    } else {
      last_paging_key_ = 0;
      paging_keystroke_count_ = 0;
    }

    PerformKeyBinding(binding);

    return kAccepted;
  }

  // 未執行綁定，清除記錄
  if (!key_event.release()) {
    last_paging_key_ = 0;
    paging_keystroke_count_ = 0;
  }
  return kNoop;
}

void KeyBinder::PerformKeyBinding(const KeyBinding& binding) {
  if (binding.action) {
    binding.action(engine_);
  } else {
    redirecting_ = true;
    for (const KeyEvent& key_event : binding.target) {
      engine_->ProcessKey(key_event);
    }
    redirecting_ = false;
  }
}

void KeyBinder::LoadConfig() {
  if (!engine_)
    return;
  Config* config = engine_->schema()->config();
  if (auto bindings = config->GetList("key_binder/bindings"))
    key_bindings_->LoadBindings(bindings);
}

bool KeyBinder::ReinterpretPagingKey(const KeyEvent& key_event) {
  if (key_event.release() || key_event.modifier() != 0)
    return false;

  int ch = key_event.keycode();

  // 只有恰好只按過一次翻頁鍵，且該鍵是 '.'，後接字母才算敲網址
  if (paging_keystroke_count_ == 1 && last_paging_key_ == '.' &&
      std::isalpha(ch)) {
    Context* ctx = engine_->context();
    const string& input(ctx->input());
    if (!input.empty() && input.back() != '.') {
      LOG(INFO) << "reinterpreted paging key: '" << (char)last_paging_key_
                << "', successor: '" << (char)ch << "'";
      ctx->PushInput(last_paging_key_);
      last_paging_key_ = 0;
      paging_keystroke_count_ = 0;
      return true;
    }
  }

  return false;
}

}  // namespace rime
