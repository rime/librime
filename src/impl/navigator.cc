// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-20 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/impl/navigator.h>

namespace rime {

Processor::Result Navigator::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.release())
    return kNoop;
  Context *ctx = engine_->context();
  if (!ctx->IsComposing())
    return kNoop;
  int ch = key_event.keycode();
  if (ch == XK_Left || ch == XK_KP_Left) {
    Left(ctx);
    return kAccepted;
  }
  if (ch == XK_Right || ch == XK_KP_Right) {
    Right(ctx);
    return kAccepted;
  }
  if (ch == XK_Home || ch == XK_KP_Home) {
    Home(ctx);
    return kAccepted;
  }
  if (ch == XK_End || ch == XK_KP_End) {
    End(ctx);
    return kAccepted;
  }
  // not handled
  return kNoop;
}

bool Navigator::Left(Context *ctx) {
  EZDBGONLYLOGGERFUNCTRACKER;
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return End(ctx);
  ctx->set_caret_pos(caret_pos - 1);
  return true;
}

bool Navigator::Right(Context *ctx) {
  EZDBGONLYLOGGERFUNCTRACKER;
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos >= ctx->input().length())
    return Home(ctx);
  ctx->set_caret_pos(caret_pos + 1);
  return true;
}

bool Navigator::Home(Context *ctx) {
  EZDBGONLYLOGGERFUNCTRACKER;
  size_t caret_pos = ctx->caret_pos();
  Composition *comp = ctx->composition();
  if (!comp->empty()) {
    size_t confirmed_pos = caret_pos;
    BOOST_REVERSE_FOREACH(const Segment &seg, *comp) {
      if (seg.status >= Segment::kSelected) {
        break;
      }
      confirmed_pos = seg.start;
    }
    if (confirmed_pos < caret_pos) {
      ctx->set_caret_pos(confirmed_pos);
      return true;
    }
  }
  ctx->set_caret_pos(0);
  return true;
}

bool Navigator::End(Context *ctx) {
  EZDBGONLYLOGGERFUNCTRACKER;
  size_t end_pos = ctx->input().length();
  if (ctx->caret_pos() != end_pos) {
    ctx->set_caret_pos(end_pos);
    return true;
  }
  return false;
}

}  // namespace rime
