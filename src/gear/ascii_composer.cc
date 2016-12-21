//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/gear/ascii_composer.h>

namespace rime {

static struct AsciiModeSwitchStyleDefinition {
  const char* repr;
  AsciiModeSwitchStyle style;
} ascii_mode_switch_styles[] = {
  { "inline_ascii", kAsciiModeSwitchInline },
  { "commit_text", kAsciiModeSwitchCommitText },
  { "commit_code", kAsciiModeSwitchCommitCode },
  { "clear", kAsciiModeSwitchClear },
  { NULL, kAsciiModeSwitchNoop }
};

static void load_bindings(const an<ConfigMap>& src,
                          AsciiModeSwitchKeyBindings* dest) {
  if (!src)
    return;
  for (auto it = src->begin(); it != src->end(); ++it) {
    auto value = As<ConfigValue>(it->second);
    if (!value)
      continue;
    auto* p = ascii_mode_switch_styles;
    while (p->repr && p->repr != value->str())
      ++p;
    if (p->style == kAsciiModeSwitchNoop)
      continue;
    KeyEvent ke;
    if (!ke.Parse(it->first) || ke.modifier() != 0) {
      LOG(WARNING) << "invalid ascii mode switch key: " << it->first;
      continue;
    }
    // save binding
    (*dest)[ke.keycode()] = p->style;
  }
}

AsciiComposer::AsciiComposer(const Ticket& ticket)
    : Processor(ticket) {
  LoadConfig(ticket.schema);
}

AsciiComposer::~AsciiComposer() {
  connection_.disconnect();
}

ProcessResult AsciiComposer::ProcessKeyEvent(const KeyEvent& key_event) {
  if ((key_event.shift() && key_event.ctrl()) ||
      key_event.alt() || key_event.super()) {
    shift_key_pressed_ = ctrl_key_pressed_ = false;
    return kNoop;
  }
  if (caps_lock_switch_style_ != kAsciiModeSwitchNoop) {
    ProcessResult result = ProcessCapsLock(key_event);
    if (result != kNoop)
      return result;
  }
  int ch = key_event.keycode();
  if (ch == XK_Eisu_toggle) {  // Alphanumeric toggle
    if (!key_event.release()) {
      shift_key_pressed_ = ctrl_key_pressed_ = false;
      ToggleAsciiModeWithKey(ch);
      return kAccepted;
    }
    else {
      return kRejected;
    }
  }
  bool is_shift = (ch == XK_Shift_L || ch == XK_Shift_R);
  bool is_ctrl = (ch == XK_Control_L || ch == XK_Control_R);
  if (is_shift || is_ctrl) {
    if (key_event.release()) {
      if (shift_key_pressed_ || ctrl_key_pressed_) {
        auto now = std::chrono::steady_clock::now();
        if (now < toggle_expired_) {
          ToggleAsciiModeWithKey(ch);
        }
        shift_key_pressed_ = ctrl_key_pressed_ = false;
        return kRejected;
      }
    }
    else if (!(shift_key_pressed_ || ctrl_key_pressed_)) {  // first key down
      if (is_shift)
        shift_key_pressed_ = true;
      else
        ctrl_key_pressed_ = true;
      // will not toggle unless the toggle key is released shortly
      const auto toggle_duration_limit = std::chrono::milliseconds(500);
      auto now = std::chrono::steady_clock::now();
      toggle_expired_= now + toggle_duration_limit;
    }
    return kNoop;
  }
  // other keys
  shift_key_pressed_ = ctrl_key_pressed_ = false;
  if (key_event.ctrl()) {
    return kNoop;  // possible key binding Control+x
  }
  Context* ctx = engine_->context();
  bool ascii_mode = ctx->get_option("ascii_mode");
  if (ascii_mode) {
    if (!ctx->IsComposing()) {
      return kRejected;  // direct commit
    }
    // edit inline ascii string
    if (!key_event.release() && ch >= 0x20 && ch < 0x80) {
      ctx->PushInput(ch);
      return kAccepted;
    }
  }
  return kNoop;
}

ProcessResult AsciiComposer::ProcessCapsLock(const KeyEvent& key_event) {
  int ch = key_event.keycode();
  if (ch == XK_Caps_Lock) {
    if (!key_event.release()) {
      shift_key_pressed_ = ctrl_key_pressed_ = false;
      // temprarily disable good-old (uppercase) Caps Lock as mode switch key
      // in case the user switched to ascii mode with other keys, eg. with Shift
      if (good_old_caps_lock_ && !toggle_with_caps_) {
        Context* ctx = engine_->context();
        bool ascii_mode = ctx->get_option("ascii_mode");
        if (ascii_mode) {
          return kRejected;
        }
      }
      toggle_with_caps_ = !key_event.caps();
      // NOTE: for Linux, Caps Lock modifier is clear when we are about to
      // turn it on; for Windows it is the opposite:
      // Caps Lock modifier has been set before we process VK_CAPITAL.
      // here we assume IBus' behavior and invert caps with ! operation.
      SwitchAsciiMode(!key_event.caps(), caps_lock_switch_style_);
      return kAccepted;
    }
    else {
      return kRejected;
    }
  }
  if (key_event.caps()) {
    if (!good_old_caps_lock_ &&
        !key_event.release() && !key_event.ctrl() &&
        isascii(ch) && isalpha(ch)) {
      // output ascii characters ignoring Caps Lock
      if (islower(ch))
        ch = toupper(ch);
      else if (isupper(ch))
        ch = tolower(ch);
      engine_->CommitText(string(1, ch));
      return kAccepted;
    }
    else {
      return kRejected;
    }
  }
  return kNoop;
}

void AsciiComposer::LoadConfig(Schema* schema) {
  bindings_.clear();
  caps_lock_switch_style_ = kAsciiModeSwitchNoop;
  good_old_caps_lock_ = false;
  if (!schema)
    return;
  the<Config> preset_config(
      Config::Require("config")->Create("default"));
  if (preset_config) {
    preset_config->GetBool("ascii_composer/good_old_caps_lock",
                           &good_old_caps_lock_);
  }
  Config* config = schema->config();
  auto bindings = config->GetMap("ascii_composer/switch_key");
  if (!bindings) {
    if (!preset_config) {
      LOG(ERROR) << "Error importing preset ascii bindings.";
      return;
    }
    bindings = preset_config->GetMap("ascii_composer/switch_key");
    if (!bindings) {
      LOG(WARNING) << "missing preset ascii bindings.";
      return;
    }
  }
  load_bindings(bindings, &bindings_);
  auto it = bindings_.find(XK_Caps_Lock);
  if (it != bindings_.end()) {
    caps_lock_switch_style_ = it->second;
    if (caps_lock_switch_style_ == kAsciiModeSwitchInline) {  // can't do that
      caps_lock_switch_style_ = kAsciiModeSwitchClear;
    }
  }
}

bool AsciiComposer::ToggleAsciiModeWithKey(int key_code) {
  auto it = bindings_.find(key_code);
  if (it == bindings_.end())
    return false;
  AsciiModeSwitchStyle style = it->second;
  Context* ctx = engine_->context();
  bool ascii_mode = !ctx->get_option("ascii_mode");
  SwitchAsciiMode(ascii_mode, style);
  toggle_with_caps_ = (key_code == XK_Caps_Lock);
  return true;
}

void AsciiComposer::SwitchAsciiMode(bool ascii_mode,
                                    AsciiModeSwitchStyle style) {
  DLOG(INFO) << "ascii mode: " << ascii_mode << ", switch style: " << style;
  Context* ctx = engine_->context();
  if (ctx->IsComposing()) {
    connection_.disconnect();
    // temporary ascii mode in desired manner
    if (style == kAsciiModeSwitchInline) {
      LOG(INFO) << "converting current composition to "
                << (ascii_mode ? "ascii" : "non-ascii") << " mode.";
      if (ascii_mode) {
        connection_ = ctx->update_notifier().connect(
            [this](Context* ctx) { OnContextUpdate(ctx); });
      }
    }
    else if (style == kAsciiModeSwitchCommitText) {
      ctx->ConfirmCurrentSelection();
    }
    else if (style == kAsciiModeSwitchCommitCode) {
      ctx->ClearNonConfirmedComposition();
      ctx->Commit();
    }
    else if (style == kAsciiModeSwitchClear) {
      ctx->Clear();
    }
  }
  // refresh non-confirmed composition with new mode
  ctx->set_option("ascii_mode", ascii_mode);
}

void AsciiComposer::OnContextUpdate(Context* ctx) {
  if (!ctx->IsComposing()) {
    connection_.disconnect();
    // quit temporary ascii mode
    ctx->set_option("ascii_mode", false);
  }
}

}  // namespace rime
