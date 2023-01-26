//
// Copyright RIME Developers
// Distributed under the BSD License
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
#include <rime/gear/selector.h>

namespace rime {

static Selector::ActionDef selector_actions[] = {
  { "previous_candidate", &Selector::PreviousCandidate },
  { "next_candidate", &Selector::NextCandidate },
  { "previous_page", &Selector::PreviousPage },
  { "next_page", &Selector::NextPage },
  { "home", &Selector::Home },
  { "end", &Selector::End },
  Selector::kActionNoop,
};

Selector::Selector(const Ticket& ticket)
  : Processor(ticket),
    KeyBindingProcessor(selector_actions)
{
  // default key bindings
  {
    auto& keymap = get_keymap(Horizontal | Stacked);
    keymap.Bind({XK_Up, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_KP_Up, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_Down, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_KP_Down, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_KP_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_KP_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_Home, 0}, &Selector::Home);
    keymap.Bind({XK_KP_Home, 0}, &Selector::Home);
    keymap.Bind({XK_End, 0}, &Selector::End);
    keymap.Bind({XK_KP_End, 0}, &Selector::End);
  }
  {
    auto& keymap = get_keymap(Horizontal | Linear);
    keymap.Bind({XK_Left, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_KP_Left, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_Right, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_KP_Right, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_Up, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_KP_Up, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_Down, 0}, &Selector::NextPage);
    keymap.Bind({XK_KP_Down, 0}, &Selector::NextPage);
    keymap.Bind({XK_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_KP_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_KP_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_Home, 0}, &Selector::Home);
    keymap.Bind({XK_KP_Home, 0}, &Selector::Home);
    keymap.Bind({XK_End, 0}, &Selector::End);
    keymap.Bind({XK_KP_End, 0}, &Selector::End);
  }
  {
    auto& keymap = get_keymap(Vertical | Stacked);
    keymap.Bind({XK_Right, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_KP_Right, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_Left, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_KP_Left, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_KP_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_KP_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_Home, 0}, &Selector::Home);
    keymap.Bind({XK_KP_Home, 0}, &Selector::Home);
    keymap.Bind({XK_End, 0}, &Selector::End);
    keymap.Bind({XK_KP_End, 0}, &Selector::End);
  }
  {
    auto& keymap = get_keymap(Vertical | Linear);
    keymap.Bind({XK_Up, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_KP_Up, 0}, &Selector::PreviousCandidate);
    keymap.Bind({XK_Down, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_KP_Down, 0}, &Selector::NextCandidate);
    keymap.Bind({XK_Right, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_KP_Right, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_Left, 0}, &Selector::NextPage);
    keymap.Bind({XK_KP_Left, 0}, &Selector::NextPage);
    keymap.Bind({XK_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_KP_Prior, 0}, &Selector::PreviousPage);
    keymap.Bind({XK_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_KP_Next, 0}, &Selector::NextPage);
    keymap.Bind({XK_Home, 0}, &Selector::Home);
    keymap.Bind({XK_KP_Home, 0}, &Selector::Home);
    keymap.Bind({XK_End, 0}, &Selector::End);
    keymap.Bind({XK_KP_End, 0}, &Selector::End);
  }

  Config* config = engine_->schema()->config();
  LoadConfig(config, "selector", Horizontal | Stacked);
  LoadConfig(config, "selector/linear", Horizontal | Linear);
  LoadConfig(config, "selector/vertical", Vertical | Stacked);
  LoadConfig(config, "selector/vertical/linear", Vertical | Linear);
}

inline static bool is_vertical_text(Context* ctx) {
  return ctx->get_option("_vertical");
}

inline static bool is_linear_layout(Context* ctx) {
  return ctx->get_option("_linear") ||
    // Deprecated. equivalent to {_linear: true, _vertical: false}
    ctx->get_option("_horizontal");
}

ProcessResult Selector::ProcessKeyEvent(const KeyEvent& key_event) {
  if (key_event.release()) {
    last_key_ = 0;
    key_repeat_ = 0;
    return kNoop;
  }
  if (key_event.alt() || key_event.super())
    return kNoop;

  Context* ctx = engine_->context();
  if (ctx->composition().empty())
    return kNoop;
  Segment& current_segment(ctx->composition().back());
  if (!current_segment.menu || current_segment.HasTag("raw"))
    return kNoop;

  TextOrientation text_orientation =
    is_vertical_text(ctx) ? Vertical : Horizontal;
  CandidateListLayout candidate_list_layout =
    is_linear_layout(ctx) ? Linear : Stacked;
  auto result = KeyBindingProcessor::ProcessKeyEvent(
    key_event, ctx, text_orientation | candidate_list_layout);

  int ch = key_event.keycode();
  if (result != kNoop) {
    if (last_key_ == ch) {
      ++key_repeat_;
    } else {
      last_key_ = ch;
      key_repeat_ = 1;
    }
    return result;
  }

  int index = -1;
  const string& select_keys(engine_->schema()->select_keys());
  if (!select_keys.empty() &&
      !key_event.ctrl() &&
      ch >= 0x20 && ch < 0x7f) {
    size_t pos = select_keys.find((char)ch);
    if (pos != string::npos) {
      index = static_cast<int>(pos);
    }
  }
  else if (ch >= XK_0 && ch <= XK_9)
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

bool Selector::PreviousPage(Context* ctx) {
  Composition& comp = ctx->composition();
  if (comp.empty())
    return false;
  int page_size = engine_->schema()->page_size();
  int selected_index = comp.back().selected_index;
  int index = selected_index < page_size ? 0 : selected_index - page_size;
  comp.back().selected_index = index;
  comp.back().tags.insert("paging");
  return true;
}

bool Selector::NextPage(Context* ctx) {
  Composition& comp = ctx->composition();
  if (comp.empty() || !comp.back().menu)
    return false;
  int page_size = engine_->schema()->page_size();
  int index = comp.back().selected_index + page_size;
  int page_start = (index / page_size) * page_size;
  int candidate_count = comp.back().menu->Prepare(page_start + page_size);
  if (candidate_count <= page_start) {
    bool page_down_cycle = engine_->schema()->page_down_cycle();
    if (page_down_cycle) {// Cycle back to page 1 if true
      index = 0;
    } else {
      // no-op; consume the key event so that page down is not sent to the app.
      return true;
    }
  } else if (index >= candidate_count) {
    index = candidate_count - 1;
  }
  comp.back().selected_index = index;
  comp.back().tags.insert("paging");
  return true;
}

inline static bool caret_at_end_of_input(Context* ctx) {
  return ctx->caret_pos() >= ctx->input().length();
}

bool Selector::PreviousCandidate(Context* ctx) {
  if (is_linear_layout(ctx) && !caret_at_end_of_input(ctx)) {
    // let navigator handle the arrow key.
    return false;
  }
  Composition& comp = ctx->composition();
  if (comp.empty())
    return false;
  int index = comp.back().selected_index;
  if (index <= 0) {
    // in case of linear layout, fall back to navigator;
    // repeated key press should be handled by the same processor.
    return !is_linear_layout(ctx) || key_repeat_ > 0;
  }
  comp.back().selected_index = index - 1;
  comp.back().tags.insert("paging");
  return true;
}

bool Selector::NextCandidate(Context* ctx) {
  if (is_linear_layout(ctx) && !caret_at_end_of_input(ctx)) {
    // let navigator handle the arrow key.
    return false;
  }
  Composition& comp = ctx->composition();
  if (comp.empty() || !comp.back().menu)
    return false;
  int index = comp.back().selected_index + 1;
  int candidate_count = comp.back().menu->Prepare(index + 1);
  if (candidate_count <= index)
    return true;
  comp.back().selected_index = index;
  comp.back().tags.insert("paging");
  return true;
}

bool Selector::Home(Context* ctx) {
  if (ctx->composition().empty())
    return false;
  Segment& seg(ctx->composition().back());
  if (seg.selected_index > 0) {
    seg.selected_index = 0;
    return true;
  }
  // let navigator handle the key event.
  return false;
}

bool Selector::End(Context* ctx) {
  if (ctx->caret_pos() < ctx->input().length()) {
    // navigator should handle this
    return false;
  }
  // this is cool:
  return Home(ctx);
}

bool Selector::SelectCandidateAt(Context* ctx, int index) {
  Composition& comp = ctx->composition();
  if (comp.empty())
    return false;
  int page_size = engine_->schema()->page_size();
  if (index >= page_size)
    return false;
  int selected_index = comp.back().selected_index;
  int page_start = (selected_index / page_size) * page_size;
  return ctx->Select(page_start + index);
}

}  // namespace rime
