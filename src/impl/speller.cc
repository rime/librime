// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-27 GONG Chen <chen.sst@gmail.com>
//
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
                                   alphabet_(kRimeAlphabet) {
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiter_);
    config->GetString("speller/initials", &initials_);
  }
  if (initials_.empty()) initials_ = alphabet_;
}

Processor::Result Speller::ProcessKeyEvent(
    const KeyEvent &key_event) {
  if (key_event.release() || key_event.ctrl() || key_event.alt())
    return kNoop;
  int ch = key_event.keycode();
  Context *ctx = engine_->context();
  if (ctx->IsComposing()) {
    if (initials_.find(ch) == std::string::npos)
      return kNoop;
  }
  else {
    if (alphabet_.find(ch) == std::string::npos)
      return kNoop;
  }
  EZLOGGERPRINT("Add to input: '%c', %d, '%s'", ch, key_event.keycode(), key_event.repr().c_str());
  ctx->PushInput(key_event.keycode());
  ctx->ConfirmPreviousSelection();  // so that next BackSpace does not revert previous selection
  return kAccepted;
}

}  // namespace rime
