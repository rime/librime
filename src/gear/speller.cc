//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-10-27 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/gear/speller.h>

static const char kRimeAlphabet[] = "zyxwvutsrqponmlkjihgfedcba";

namespace rime {

static inline bool belongs_to(char ch, const std::string& charset) {
  return charset.find(ch) != std::string::npos;
}

static bool is_auto_selectable(const shared_ptr<Candidate>& cand,
                               const std::string& input,
                               const std::string& delimiters) {
  return
      // reaches end of input
      cand->end() == input.length() &&
      // is table entry
      Candidate::GetGenuineCandidate(cand)->type() == "table" &&
      // no delimiters
      input.find_first_of(delimiters, cand->start()) == std::string::npos;
}

static bool expecting_an_initial(Context* ctx,
                                 const std::string& alphabet,
                                 const std::string& finals) {
  size_t caret_pos = ctx->caret_pos();
  if (caret_pos == 0)
    return true;
  const std::string& input(ctx->input());
  //assert(input.length() >= caret_pos);
  char previous_char = input[caret_pos - 1];
  return belongs_to(previous_char, finals) ||
         !belongs_to(previous_char, alphabet);
}

Speller::Speller(const Ticket& ticket) : Processor(ticket),
                                         alphabet_(kRimeAlphabet) {
  if (Config* config = engine_->schema()->config()) {
    config->GetString("speller/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiters_);
    config->GetString("speller/initials", &initials_);
    config->GetString("speller/finals", &finals_);
    config->GetInt("speller/max_code_length", &max_code_length_);
    config->GetBool("speller/auto_select", &auto_select_);
    if (!config->GetBool("speller/auto_select_unique_candidate",
                         &auto_select_unique_candidate_)) {
      auto_select_unique_candidate_ = auto_select_;
    }
    config->GetBool("speller/use_space", &use_space_);
  }
  if (initials_.empty()) {
    initials_ = alphabet_;
  }
}

ProcessResult Speller::ProcessKeyEvent(const KeyEvent& key_event) {
  if (key_event.release() || key_event.ctrl() || key_event.alt())
    return kNoop;
  int ch = key_event.keycode();
  if (ch < 0x20 || ch >= 0x7f)  // not a valid key for spelling
    return kNoop;
  if (ch == XK_space && (!use_space_ || key_event.shift()))
    return kNoop;
  if (!belongs_to(ch, alphabet_) && !belongs_to(ch, delimiters_))
    return kNoop;
  Context* ctx = engine_->context();
  bool is_initial = belongs_to(ch, initials_);
  if (!is_initial &&
      expecting_an_initial(ctx, alphabet_, finals_)) {
    return kNoop;
  }
  if (is_initial &&  // an initial triggers auto-select
      max_code_length_ > 0 &&  // at a fixed code length
      ctx->HasMenu()) {
    const Segment& seg(ctx->composition()->back());
    if (auto cand = seg.GetSelectedCandidate()) {
      int code_length = static_cast<int>(cand->end() - cand->start());
      if (code_length == max_code_length_ &&  // exceeds max code length
          is_auto_selectable(cand, ctx->input(), delimiters_)) {
        ctx->ConfirmCurrentSelection();
      }
    }
  }
  Segment previous_segment;
  if (auto_select_ && ctx->HasMenu()) {
    previous_segment = ctx->composition()->back();
  }
  DLOG(INFO) << "add to input: '" << (char)ch << "', " << key_event.repr();
  ctx->PushInput(key_event.keycode());
  ctx->ConfirmPreviousSelection();  // so that next BackSpace won't revert
                                    // previous selection
  if (auto_select_unique_candidate_ && ctx->HasMenu()) {
    const Segment& seg(ctx->composition()->back());
    bool unique_candidate = seg.menu->Prepare(2) == 1;
    if (unique_candidate &&
        is_auto_selectable(seg.GetSelectedCandidate(),
                           ctx->input(), delimiters_)) {
      DLOG(INFO) << "auto-select unique candidate.";
      ctx->ConfirmCurrentSelection();
      return kAccepted;
    }
  }
  if (auto_select_ && !ctx->HasMenu() && previous_segment.menu) {
    if (is_auto_selectable(previous_segment.GetSelectedCandidate(),
                           ctx->input().substr(0, previous_segment.end),
                           delimiters_)) {
      DLOG(INFO) << "auto-select previous word";
      ctx->composition()->pop_back();
      ctx->composition()->push_back(previous_segment);
      ctx->ConfirmCurrentSelection();
      return kAccepted;
    }
  }
  return kAccepted;
}

}  // namespace rime
