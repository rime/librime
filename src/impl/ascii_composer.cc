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

AsciiComposer::AsciiComposer(Engine *engine)
    : Processor(engine), shift_key_pressed_(false) {
}

Processor::Result AsciiComposer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.ctrl() || key_event.alt()) {
    shift_key_pressed_ = false;
    return kNoop;
  }
  int ch = key_event.keycode();
  if (ch == XK_Shift_L || ch == XK_Shift_R) {
    if (key_event.release()) {
      if (shift_key_pressed_) {
        ToggleAsciiMode(ch);
        shift_key_pressed_ = false;
        return kRejected;
      }
    }
    else {
      shift_key_pressed_ = true;
    }
    return kNoop;
  }
  // other keys
  shift_key_pressed_ = false;
  Context *ctx = engine_->context();
  bool ascii_mode = ctx->get_option("ascii_mode");
  if (ascii_mode) {
    if (!ctx->IsComposing()) {
      return kRejected;  // direct commit
    }
    if (!key_event.release() && (ch >= 0x20 && ch < 0x80)) {
      ctx->PushInput(ch);
      return kAccepted;
    }
  }
  return kNoop;
}

void AsciiComposer::ToggleAsciiMode(int key_code) {
  Context *ctx = engine_->context();
  bool ascii_mode = !ctx->get_option("ascii_mode");
  if (ctx->IsComposing()) {
    connection_.disconnect();
    if (key_code == XK_Shift_L) {
      EZLOGGERPRINT("converting current composition to %s mode.",
                    ascii_mode ? "ascii" : "non-ascii");
      if (ascii_mode) {
        connection_ = ctx->update_notifier().connect(
            boost::bind(&AsciiComposer::OnContextUpdate, this, _1));
      }
    }
    else if (key_code == XK_Shift_R) {
      ctx->ConfirmCurrentSelection();
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
