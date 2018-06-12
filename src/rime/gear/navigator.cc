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
#include <rime/schema.h>
#include <rime/gear/navigator.h>
#include <rime/gear/translator_commons.h>

namespace rime {

static Navigator::ActionDef navigation_actions[] = {
  { "rewind", &Navigator::Rewind },
  { "left_by_char", &Navigator::LeftByChar },
  { "right_by_char", &Navigator::RightByChar },
  { "left_by_syllable", &Navigator::LeftBySyllable },
  { "right_by_syllable", &Navigator::RightBySyllable },
  { "home", &Navigator::Home },
  { "end", &Navigator::End },
  Navigator::kActionNoop
};

Navigator::Navigator(const Ticket& ticket)
    : Processor(ticket), KeyBindingProcessor<Navigator>(navigation_actions) {
  // Default key binding.
  Bind({XK_Left, 0}, &Navigator::Rewind);
  Bind({XK_Left, kControlMask}, &Navigator::LeftBySyllable);
  Bind({XK_KP_Left, 0}, &Navigator::LeftByChar);
  Bind({XK_Right, 0}, &Navigator::RightByChar);
  Bind({XK_Right, kControlMask}, &Navigator::RightBySyllable);
  Bind({XK_KP_Right, 0}, &Navigator::RightByChar);
  Bind({XK_Home, 0}, &Navigator::Home);
  Bind({XK_KP_Home, 0}, &Navigator::Home);
  Bind({XK_End, 0}, &Navigator::End);
  Bind({XK_KP_End, 0}, &Navigator::End);

  Config* config = engine_->schema()->config();
  KeyBindingProcessor::LoadConfig(config, "navigator");
}

ProcessResult Navigator::ProcessKeyEvent(const KeyEvent& key_event) {
  if (key_event.release())
    return kNoop;
  Context* ctx = engine_->context();
  if (!ctx->IsComposing())
    return kNoop;
  return KeyBindingProcessor::ProcessKeyEvent(key_event, ctx);
}

void Navigator::LeftBySyllable(Context* ctx) {
  BeginMove(ctx);
  size_t confirmed_pos = ctx->composition().GetConfirmedPosition();
  JumpLeft(ctx, confirmed_pos) || GoToEnd(ctx);
}

void Navigator::LeftByChar(Context* ctx) {
  BeginMove(ctx);
  MoveLeft(ctx) || GoToEnd(ctx);
}

void Navigator::Rewind(Context* ctx) {
  BeginMove(ctx);
  // take a jump leftwards when there are multiple spans,
  // but not from the middle of a span.
  (
      spans_.Count() > 1 && spans_.HasVertex(ctx->caret_pos())
      ? JumpLeft(ctx) : MoveLeft(ctx)
  ) || GoToEnd(ctx);
}

void Navigator::RightBySyllable(Context* ctx) {
  BeginMove(ctx);
  size_t confirmed_pos = ctx->composition().GetConfirmedPosition();
  JumpRight(ctx, confirmed_pos) || GoToEnd(ctx);
}

void Navigator::RightByChar(Context* ctx) {
  BeginMove(ctx);
  MoveRight(ctx) || GoHome(ctx);
}

void Navigator::Home(Context* ctx) {
  BeginMove(ctx);
  GoHome(ctx);
}

void Navigator::End(Context* ctx) {
  BeginMove(ctx);
  GoToEnd(ctx);
}

void Navigator::BeginMove(Context* ctx) {
  ctx->ConfirmPreviousSelection();
  // update spans
  if (input_ != ctx->input() || ctx->caret_pos() > spans_.end()) {
    input_ = ctx->input();
    spans_.Clear();
    for (const auto &seg : ctx->composition()) {
      if (auto phrase = As<Phrase>(
              Candidate::GetGenuineCandidate(
                  seg.GetSelectedCandidate()))) {
        spans_.AddSpans(phrase->spans());
      }
      spans_.AddSpan(seg.start, seg.end);
    }
  }
}

bool Navigator::JumpLeft(Context* ctx, size_t start_pos) {
  DLOG(INFO) << "jump left.";
  size_t caret_pos = ctx->caret_pos();
  size_t stop = spans_.PreviousStop(caret_pos);
  if (stop < start_pos) {
    stop = ctx->input().length();  // rewind
  }
  if (stop != caret_pos) {
    ctx->set_caret_pos(stop);
    return true;
  }
  return false;
}

bool Navigator::JumpRight(Context* ctx, size_t start_pos) {
  DLOG(INFO) << "jump right.";
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == ctx->input().length()) {
    caret_pos = start_pos;  // rewind
  }
  size_t stop = spans_.NextStop(caret_pos);
  if (stop != caret_pos) {
    ctx->set_caret_pos(stop);
    return true;
  }
  return false;
}

bool Navigator::MoveLeft(Context* ctx) {
  DLOG(INFO) << "navigate left.";
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return false;
  ctx->set_caret_pos(caret_pos - 1);
  return true;
}

bool Navigator::MoveRight(Context* ctx) {
  DLOG(INFO) << "navigate right.";
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos >= ctx->input().length())
    return false;
  ctx->set_caret_pos(caret_pos + 1);
  return true;
}

bool Navigator::GoHome(Context* ctx) {
  DLOG(INFO) << "navigate home.";
  size_t caret_pos = ctx->caret_pos();
  const Composition& comp = ctx->composition();
  if (!comp.empty()) {
    size_t confirmed_pos = caret_pos;
    for (const Segment& seg : boost::adaptors::reverse(comp)) {
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

bool Navigator::GoToEnd(Context* ctx) {
  DLOG(INFO) << "navigate end.";
  size_t end_pos = ctx->input().length();
  if (ctx->caret_pos() != end_pos) {
    ctx->set_caret_pos(end_pos);
    return true;
  }
  return false;
}

}  // namespace rime
