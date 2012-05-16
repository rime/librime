// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/impl/ascii_composer.h>

namespace rime {

static struct AsciiModeSwitchStyleDefinition {
  const char* repr;
  AsciiModeSwitchStyle style;
} ascii_mode_switch_styles[] = {
  { "inline_ascii", kAsciiModeSwitchInline },
  { "commit_text", kAsciiModeSwitchCommitText },
  { "commit_code", kAsciiModeSwitchCommitCode },
  { NULL, kAsciiModeSwitchNoop }
};

static void load_bindings(const ConfigMapPtr &src,
                          AsciiModeSwitchKeyBindings* dest) {
  if (!src) return;
  for (ConfigMap::Iterator it = src->begin();
       it != src->end(); ++it) {
    ConfigValuePtr value(As<ConfigValue>(it->second));
    if (!value) continue;
    AsciiModeSwitchStyleDefinition* p = ascii_mode_switch_styles;
    while (p->repr && p->repr != value->str())
      ++p;
    if (p->style == kAsciiModeSwitchNoop)
      continue;
    KeyEvent ke;
    if (!ke.Parse(it->first) || ke.modifier() != 0) {
      EZLOGGERPRINT("Warning: invalid ascii mode switch key: %s.",
                    it->first.c_str());
      continue;
    }
    // save binding
    (*dest)[ke.keycode()] = p->style;
  }
}

AsciiComposer::AsciiComposer(Engine *engine)
    : Processor(engine), shift_key_pressed_(false), ctrl_key_pressed_(false) {
  LoadConfig(engine->schema());
}

Processor::Result AsciiComposer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.shift() && key_event.ctrl() || key_event.alt()) {
    shift_key_pressed_ = ctrl_key_pressed_ = false;
    return kNoop;
  }
  int ch = key_event.keycode();
  bool is_shift = (ch == XK_Shift_L || ch == XK_Shift_R);
  bool is_ctrl = (ch == XK_Control_L || ch == XK_Control_R);
  if (is_shift || is_ctrl) {
    if (key_event.release()) {
      if (shift_key_pressed_ || ctrl_key_pressed_) {
        ToggleAsciiMode(ch);
        shift_key_pressed_ = ctrl_key_pressed_ = false;
        return kRejected;
      }
    }
    else {  // start pressing
      if (is_shift)
        shift_key_pressed_ = true;
      else
        ctrl_key_pressed_ = true;
    }
    return kNoop;
  }
  // other keys
  shift_key_pressed_ = ctrl_key_pressed_ = false;
  Context *ctx = engine_->context();
  bool ascii_mode = ctx->get_option("ascii_mode");
  if (ascii_mode) {
    if (!ctx->IsComposing()) {
      return kRejected;  // direct commit
    }
    // edit inline ascii
    if (!key_event.release() && (ch >= 0x20 && ch < 0x80)) {
      ctx->PushInput(ch);
      return kAccepted;
    }
  }
  return kNoop;
}

void AsciiComposer::LoadConfig(Schema* schema) {
  if (!schema) return;
  Config *config = schema->config();
  ConfigMapPtr  bindings = config->GetMap("ascii_composer/switch_key");
  if (!bindings) {
    scoped_ptr<Config> preset_config(
        Config::Require("config")->Create("default"));
    if (!preset_config) {
      EZLOGGERPRINT("Error importing preset ascii bindings.");
      return;
    }
    bindings = preset_config->GetMap("ascii_composer/switch_key");
    if (!bindings) {
      EZLOGGERPRINT("Warning: missing preset ascii bindings.");
      return;
    }
  }
  load_bindings(bindings, &bindings_);
}

void AsciiComposer::ToggleAsciiMode(int key_code) {
  AsciiModeSwitchKeyBindings::const_iterator it = bindings_.find(key_code);
  if (it == bindings_.end())
    return;
  AsciiModeSwitchStyle style = it->second;
  EZDBGONLYLOGGERVAR(style);
  Context *ctx = engine_->context();
  bool ascii_mode = !ctx->get_option("ascii_mode");
  if (ctx->IsComposing()) {
    connection_.disconnect();
    // temporary ascii mode in desired manner
    if (style == kAsciiModeSwitchInline) {
      EZLOGGERPRINT("converting current composition to %s mode.",
                    ascii_mode ? "ascii" : "non-ascii");
      if (ascii_mode) {
        connection_ = ctx->update_notifier().connect(
            boost::bind(&AsciiComposer::OnContextUpdate, this, _1));
      }
    }
    else if (style == kAsciiModeSwitchCommitText) {
      ctx->ConfirmCurrentSelection();
    }
    else if (style == kAsciiModeSwitchCommitCode) {
      ctx->ClearNonConfirmedComposition();
      ctx->Commit();
    }
  }
  // refresh non-confirmed composition with new mode
  ctx->set_option("ascii_mode", ascii_mode);
}

void AsciiComposer::OnContextUpdate(Context *ctx) {
  if (!ctx->IsComposing()) {
    connection_.disconnect();
    // quit temporary ascii mode
    ctx->set_option("ascii_mode", false);
  }
}

}  // namespace rime
