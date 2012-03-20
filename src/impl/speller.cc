// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
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
#include <rime/schema.h>
#include <rime/impl/speller.h>

static const char kRimeAlphabet[] = "zyxwvutsrqponmlkjihgfedcba";

namespace rime {

Speller::Speller(Engine *engine) : Processor(engine),
                                   alphabet_(kRimeAlphabet),
                                   max_code_length_(0) {
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiter_);
    config->GetString("speller/initials", &initials_);
    config->GetInt("speller/max_code_length", &max_code_length_);
  }
  if (initials_.empty()) initials_ = alphabet_;
}

Processor::Result Speller::ProcessKeyEvent(
    const KeyEvent &key_event) {
  if (key_event.release() || key_event.ctrl() || key_event.alt() ||
      key_event.keycode() == XK_space)
    return kNoop;
  int ch = key_event.keycode();
  Context *ctx = engine_->context();
  if (ctx->IsComposing()) {
    bool is_letter = alphabet_.find(ch) != std::string::npos;
    bool is_delimiter = delimiter_.find(ch) != std::string::npos;
    if (!is_letter && !is_delimiter)
      return kNoop;
    if (is_letter &&             // a letter may cause auto-commit
        max_code_length_ > 0 &&  // at a fixed code length
        !ctx->composition()->empty()) {
      const Segment& seg(ctx->composition()->back());
      const shared_ptr<Candidate> cand = seg.GetSelectedCandidate();
      int code_length = static_cast<int>(cand->end() - cand->start());
      if (code_length == max_code_length_ &&       // exceeds max code length
          cand->end() == ctx->input().length() &&  // reaches the end of input
          cand->type() == "zh" &&                  // not raw ascii string
          ctx->input().find_first_of(              // no delimiters
              delimiter_, cand->start()) == std::string::npos) {
        ctx->Commit();
      }
    }
  }
  else {
    if (initials_.find(ch) == std::string::npos)
      return kNoop;
  }
  EZDBGONLYLOGGERPRINT("Add to input: '%c', %d, '%s'",
                       ch, key_event.keycode(), key_event.repr().c_str());
  ctx->PushInput(key_event.keycode());
  ctx->ConfirmPreviousSelection();
  // so that next BackSpace does not revert previous selection
  return kAccepted;
}

}  // namespace rime
