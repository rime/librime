//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-06-05 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/gear/chord_composer.h>

// U+FEFF works better with MacType
static const char* kZeroWidthSpace = "\xef\xbb\xbf";  //"\xe2\x80\x8b";

namespace rime {


ChordComposer::ChordComposer(const Ticket& ticket) : Processor(ticket),
                                                     pass_thru_(false),
                                                     composing_(false) {
  if (!engine_) return;
  if (Config *config = engine_->schema()->config()) {
    config->GetString("chord_composer/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiter_);
    algebra_.Load(config->GetList("chord_composer/algebra"));
    output_format_.Load(config->GetList("chord_composer/output_format"));
    prompt_format_.Load(config->GetList("chord_composer/prompt_format"));
  }
  Context* ctx = engine_->context();
  ctx->set_option("_chord_typing", true);
  connection_ = ctx->update_notifier().connect(
      boost::bind(&ChordComposer::OnContextUpdate, this, _1));
}

ProcessResult ChordComposer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (pass_thru_)
    return kNoop;
  bool chording = !chord_.empty();
  if (key_event.shift() || key_event.ctrl() || key_event.alt()) {
    ClearChord();
    return chording ? kAccepted : kNoop;
  }
  bool is_key_up = key_event.release();
  int ch = key_event.keycode();
  Context* ctx = engine_->context();
  if (!is_key_up && ch == XK_Return) {
    if (!sequence_.empty()) {
      // commit raw input
      ctx->set_input(sequence_);
      // then the sequence should not be used again
      sequence_.clear();
    }
    ClearChord();
    return kNoop;
  }
  if (!is_key_up && ch == XK_BackSpace) {
    // invalidate sequence
    sequence_.clear();
    ClearChord();
    if (DeleteLastSyllable()) {
      return kAccepted;
    }
    return kNoop;
  }
  if (!is_key_up && ch == XK_Escape) {
    // to clear a sequence made of invalid combos
    sequence_.clear();
    ClearChord();
    return kNoop;
  }
  if (!is_key_up && ch >= 0x20 && ch <= 0x7e) {
    // save raw input
    if (!ctx->IsComposing() || !sequence_.empty()) {
      sequence_.push_back(ch);
      DLOG(INFO) << "update sequence: " << sequence_;
    }
  }
  if (alphabet_.find(ch) == std::string::npos) {
    return chording ? kAccepted : kNoop;
  }
  // in alphabet
  if (is_key_up) {
    if (pressed_.erase(ch) != 0 && pressed_.empty()) {
      FinishChord();
    }
  }
  else {  // key down
    pressed_.insert(ch);
    bool updated = chord_.insert(ch).second;
    if (updated)
      UpdateChord();
  }
  return kAccepted;
}

std::string ChordComposer::SerializeChord() {
  std::string code;
  BOOST_FOREACH(char ch, alphabet_) {
    if (chord_.find(ch) != chord_.end())
      code.push_back(ch);
  }
  algebra_.Apply(&code);
  return code;
}

void ChordComposer::UpdateChord() {
  if (!engine_) return;
  Context* ctx = engine_->context();
  Composition* comp = ctx->composition();
  std::string code(SerializeChord());
  prompt_format_.Apply(&code);
  if (comp->empty()) {
    // add an invisbile place holder segment
    // 1. to cheat ctx->IsComposing() == true
    // 2. to attach chord prompt to while chording
    ctx->PushInput(kZeroWidthSpace);
    if (comp->empty()) {
      LOG(ERROR) << "failed to update chord.";
      return;
    }
    comp->back().tags.insert("phony");
  }
  comp->back().tags.insert("chord_prompt");
  comp->back().prompt = code;
}

void ChordComposer::FinishChord() {
  if (!engine_) return;
  std::string code(SerializeChord());
  output_format_.Apply(&code);
  ClearChord();

  KeySequence sequence;
  if (sequence.Parse(code) && !sequence.empty()) {
    pass_thru_ = true;
    BOOST_FOREACH(const KeyEvent& ke, sequence) {
      if (!engine_->ProcessKeyEvent(ke)) {
        // direct commit
        engine_->CommitText(std::string(1, ke.keycode()));
        // exclude the character (eg. space) from the following sequence
        sequence_.clear();
      }
    }
    pass_thru_ = false;
  }
}

void ChordComposer::ClearChord() {
  pressed_.clear();
  chord_.clear();
  if (!engine_) return;
  Context* ctx = engine_->context();
  Composition* comp = ctx->composition();
  if (comp->empty()) {
    return;
  }
  if (comp->input().substr(comp->back().start) == kZeroWidthSpace) {
    ctx->PopInput(ctx->caret_pos() - comp->back().start);
  }
  else if (comp->back().HasTag("chord_prompt")) {
    comp->back().prompt.clear();
    comp->back().tags.erase("chord_prompt");
  }
}

bool ChordComposer::DeleteLastSyllable() {
  if (!engine_)
    return false;
  Context* ctx = engine_->context();
  Composition* comp = ctx->composition();
  const std::string& input(ctx->input());
  size_t start = comp->empty() ? 0 : comp->back().start;
  size_t caret_pos = ctx->caret_pos();
  if (input.empty() || caret_pos <= start)
    return false;
  size_t deleted = 0;
  for (; caret_pos > start; --caret_pos, ++deleted) {
    if (deleted > 0 &&
        delimiter_.find(input[caret_pos - 1]) != std::string::npos)
      break;
  }
  ctx->PopInput(deleted);
  return true;
}

void ChordComposer::OnContextUpdate(Context* ctx) {
  if (ctx->IsComposing() && ctx->input() != kZeroWidthSpace) {
    composing_ = true;
  }
  else if (composing_) {
    composing_ = false;
    sequence_.clear();
    DLOG(INFO) << "clear sequence.";
  }
}

}  // namespace rime
