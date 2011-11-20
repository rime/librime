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
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/impl/selector.h>

namespace rime {

Processor::Result Selector::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.release())
    return kNoop;
  Context *ctx = engine_->context();
  if (!ctx->composition() || ctx->composition()->empty())
    return kNoop;
  Segment &current_segment(ctx->composition()->back());
  if (!current_segment.menu || current_segment.HasTag("raw"))
    return kNoop;
  int ch = key_event.keycode();
  if (ch == XK_Prior || ch == XK_KP_Prior || 
      ch == XK_comma || ch == XK_bracketleft || ch == XK_minus) {
    PageUp(ctx);
    return kAccepted;
  }
  if (ch == XK_Next || ch == XK_KP_Next ||
      ch == XK_period || ch == XK_bracketright || ch == XK_equal) {
    PageDown(ctx);
    return kAccepted;
  }
  if (ch == XK_Up || ch == XK_KP_Up) {
    CursorUp(ctx);
    return kAccepted;
  }
  if (ch == XK_Down || ch == XK_KP_Down) {
    CursorDown(ctx);
    return kAccepted;
  }
  int index = -1;
  if (ch >= XK_0 && ch <= XK_9)
    index = ((ch - XK_0) + 9) % 10;
  else if (ch >= XK_KP_0 && ch <= XK_KP_9)
    index = ((ch - XK_KP_0) + 9) % 10;
  if (index >= 0) {
    SelectCandidateAt(ctx, index);
    return kAccepted;
  }
  // not handled
  return kNoop;
}

bool Selector::PageUp(Context *ctx) {
  Composition *comp = ctx->composition();
  if (comp->empty())
    return false;
  int page_size = engine_->schema()->page_size();
  int selected_index = comp->back().selected_index;
  int index = selected_index < page_size ? 0 : selected_index - page_size;
  comp->back().selected_index = index;
  return true;
}

bool Selector::PageDown(Context *ctx) {
  Composition *comp = ctx->composition();
  if (comp->empty() || !comp->back().menu)
    return false;
  int page_size = engine_->schema()->page_size();
  int index = comp->back().selected_index + page_size;
  int page_start = (index / page_size) * page_size;
  int candidate_count = comp->back().menu->Prepare(page_start + page_size);
  if (candidate_count <= page_start)
    return false;
  if (index >= candidate_count)
    index = candidate_count - 1;
  comp->back().selected_index = index;
  return true;
  
}

bool Selector::CursorUp(Context *ctx) {
  Composition *comp = ctx->composition();
  if (comp->empty())
    return false;
  int index = comp->back().selected_index;
  if (index <= 0)
    return false;
  comp->back().selected_index = index - 1;
  return true;
}

bool Selector::CursorDown(Context *ctx) {
  Composition *comp = ctx->composition();
  if (comp->empty() || !comp->back().menu)
    return false;
  int page_size = engine_->schema()->page_size();
  int index = comp->back().selected_index + 1;
  int candidate_count = comp->back().menu->Prepare(index + 1);
  if (candidate_count <= index)
    return false;
  comp->back().selected_index = index;
  return true;
}

bool Selector::SelectCandidateAt(Context *ctx, int index) {
  Composition *comp = ctx->composition();
  if (comp->empty())
    return false;
  int page_size = engine_->schema()->page_size();
  int selected_index = comp->back().selected_index;
  int page_start = (selected_index / page_size) * page_size;
  return ctx->Select(page_start + index);
}

}  // namespace rime
