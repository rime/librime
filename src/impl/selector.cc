// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-09-11 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/impl/selector.h>

namespace rime {

Processor::Result Selector::ProcessKeyEvent(
    const KeyEvent &key_event) {
  if (key_event.release())
    return kNoop;
  Context *ctx = engine_->context();
  if (!ctx->IsComposing())
    return kNoop;
  int ch = key_event.keycode();
  int index = -1;
  if (ch >= XK_0 && ch <= XK_9)
    index = ((ch - XK_0) + 9) % 10;
  else if (ch >= XK_KP_0 && ch <= XK_KP_9)
    index = ((ch - XK_KP_0) + 9) % 10;
  if (index >= 0) {
    int page_size = engine_->schema()->page_size();
    int selected_index = ctx->composition()->back().selected_index;
    int page_start = (selected_index / page_size) * page_size;
    ctx->Select(page_start + index);
    return kAccepted;
  }
  // not handled
  return kNoop;
}

}  // namespace rime
