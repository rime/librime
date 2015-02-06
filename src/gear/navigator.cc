//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-20 GONG Chen <chen.sst@gmail.com>
//
#include <boost/range/adaptor/reversed.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/gear/navigator.h>
#include <rime/gear/translator_commons.h>

namespace rime {

ProcessResult Navigator::ProcessKeyEvent(const KeyEvent& key_event) {
  if (key_event.release())
    return kNoop;
  Context* ctx = engine_->context();
  if (!ctx->IsComposing())
    return kNoop;
  int ch = key_event.keycode();
  if (ch == XK_Left || ch == XK_KP_Left) {
    BeginMove(ctx);
    if (key_event.ctrl() || key_event.shift()) {
      JumpLeft(ctx) || Home(ctx) || End(ctx);
    }
    else {
      JumpLeft(ctx) || Left(ctx) || End(ctx);
    }
    return kAccepted;
  }
  if (ch == XK_Right || ch == XK_KP_Right) {
    BeginMove(ctx);
    if (key_event.ctrl() || key_event.shift()) {
      JumpRight(ctx) || End(ctx) || Home(ctx);
    }
    else {
      Right(ctx) || Home(ctx);
    }
    return kAccepted;
  }
  if (ch == XK_Home || ch == XK_KP_Home) {
    BeginMove(ctx);
    Home(ctx);
    return kAccepted;
  }
  if (ch == XK_End || ch == XK_KP_End) {
    BeginMove(ctx);
    End(ctx);
    return kAccepted;
  }
  // not handled
  return kNoop;
}

void Navigator::BeginMove(Context* ctx) {
  ctx->ConfirmPreviousSelection();

  size_t caret_pos = ctx->caret_pos();
  if (!syllabification_.empty() &&
      (input_ != ctx->input() ||
       caret_pos < syllabification_.start() ||
       caret_pos > syllabification_.end())) {
    input_.clear();
    syllabification_.vertices.clear();
  }

  if (input_.empty()) {
    const Composition* comp = ctx->composition();
    if (comp && !comp->empty()) {
      if (auto phrase = As<Phrase>(
              Candidate::GetGenuineCandidate(
                  comp->back().GetSelectedCandidate()))) {
        input_ = ctx->input();
        syllabification_ = phrase->syllabification();
      }
    }
  }
}

bool Navigator::JumpLeft(Context* ctx) {
  DLOG(INFO) << "jump left.";
  size_t caret_pos = ctx->caret_pos();
  if (!syllabification_.empty()) {
    size_t stop = syllabification_.PreviousStop(caret_pos);
    if (stop == 0) {
      stop = ctx->input().length();  // rewind
    }
    if (stop != caret_pos) {
      ctx->set_caret_pos(stop);
      return true;
    }
  }
  // try to locate previous segment
  const Composition* comp = ctx->composition();
  if (!comp->empty()) {
    for (const Segment& seg : boost::adaptors::reverse(*comp)) {
      if (seg.end < caret_pos) {
        ctx->set_caret_pos(seg.end);
        return true;
      }
    }
  }
  return false;
}

bool Navigator::JumpRight(Context* ctx) {
  DLOG(INFO) << "jump right.";
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == ctx->input().length()) {
    caret_pos = 0;  // rewind
  }
  if (!syllabification_.empty()) {
    size_t stop = syllabification_.NextStop(caret_pos);
    if (stop != caret_pos) {
      ctx->set_caret_pos(stop);
      return true;
    }
  }
  return false;
}

bool Navigator::Left(Context* ctx) {
  DLOG(INFO) << "navigate left.";
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return false;
  ctx->set_caret_pos(caret_pos - 1);
  return true;
}

bool Navigator::Right(Context* ctx) {
  DLOG(INFO) << "navigate right.";
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos >= ctx->input().length())
    return false;
  ctx->set_caret_pos(caret_pos + 1);
  return true;
}

bool Navigator::Home(Context* ctx) {
  DLOG(INFO) << "navigate home.";
  size_t caret_pos = ctx->caret_pos();
  const Composition* comp = ctx->composition();
  if (!comp->empty()) {
    size_t confirmed_pos = caret_pos;
    for (const Segment& seg : boost::adaptors::reverse(*comp)) {
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
  if (caret_pos != 0) {
    ctx->set_caret_pos(0);
    return true;
  }
  return false;
}

bool Navigator::End(Context* ctx) {
  DLOG(INFO) << "navigate end.";
  size_t end_pos = ctx->input().length();
  if (ctx->caret_pos() != end_pos) {
    ctx->set_caret_pos(end_pos);
    return true;
  }
  return false;
}

}  // namespace rime
