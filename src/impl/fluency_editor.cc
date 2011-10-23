// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-23 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <rime/common.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/impl/fluency_editor.h>

namespace rime {

// treat printable characters as input
// commit with Return key
Processor::Result FluencyEditor::ProcessKeyEvent(
    const KeyEvent &key_event) {
  if (key_event.release() || key_event.ctrl() || key_event.alt())
    return kRejected;
  Context *ctx = engine_->context();
  int ch = key_event.keycode();
  if (ch == XK_space) {
    if (ctx->IsComposing()) {
      ctx->ConfirmCurrentSelection() || ctx->Commit();
      return kAccepted;
    }
    else {
      return kNoop;
    }
  }
  if (ch == XK_Return && ctx->IsComposing()) {
    ctx->Commit();
    return kAccepted;
  }
  if (ch == XK_BackSpace && ctx->IsComposing()) {
    ctx->ReopenPreviousSegment() || ctx->PopInput();
    return kAccepted;
  }
  if (ch == XK_Escape && ctx->IsComposing()) {
    ctx->Clear();
    return kAccepted;
  }
  if (ch > 20 && ch < 128) {
    EZLOGGERPRINT("Add to input: '%c', %d, '%s'", ch, key_event.keycode(), key_event.repr().c_str());
    ctx->PushInput(key_event.keycode());
    return kAccepted;
  }
  // not handled
  return kNoop;
}

}  // namespace rime
