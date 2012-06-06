// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
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
#include <rime/impl/chord_composer.h>

// U+FEFF works better with MacType
static const char* kZeroWidthSpace = "\xef\xbb\xbf";  //"\xe2\x80\x8b";

namespace rime {


ChordComposer::ChordComposer(Engine *engine) : Processor(engine),
                                               pass_thru_(false) {
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("chord_composer/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiter_);
    algebra_.Load(config->GetList("chord_composer/algebra"));
    output_format_.Load(config->GetList("chord_composer/output_format"));
    prompt_format_.Load(config->GetList("chord_composer/prompt_format"));
  }
}

Processor::Result ChordComposer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (pass_thru_)
    return kNoop;
  bool composing = !chord_.empty();
  if (key_event.shift() || key_event.ctrl() || key_event.alt()) {
    ClearChord();
    return composing ? kAccepted : kNoop;
  }
  bool is_key_up = key_event.release();
  int ch = key_event.keycode();
  if (!composing && ch == XK_BackSpace && !is_key_up) {
    if (DeleteLastSyllable())
      return kAccepted;
  }
  if (alphabet_.find(ch) == std::string::npos) {
    ClearChord();
    return composing ? kAccepted : kNoop;
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

const std::string ChordComposer::SerializeChord() {
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
  bool chord_exists = !comp->empty() && comp->back().HasTag("chord");
  bool chord_prompt = !comp->empty() && comp->back().HasTag("chord_prompt");
  if (chord_.empty()) {
    if (chord_exists) {
      ctx->PopInput(ctx->caret_pos() - comp->back().start);
    }
    else if (chord_prompt) {
      comp->back().prompt.clear();
      comp->back().tags.erase("chord_prompt");
    }
  }
  else {
    std::string code(SerializeChord());
    prompt_format_.Apply(&code);
    if (!chord_exists && !chord_prompt) {
      if (comp->empty()) {
        comp->Forward();
        ctx->PushInput(kZeroWidthSpace);
        comp->back().tags.insert("chord");
      }
      else {
        comp->back().tags.insert("chord_prompt");
      }
    }
    comp->back().prompt = code;
  }
}

void ChordComposer::FinishChord() {
  if (!engine_) return;
  std::string code(SerializeChord());
  output_format_.Apply(&code);
  ClearChord();
  
  KeySequence sequence;
  if (sequence.Parse(code)) {
    pass_thru_ = true;
    BOOST_FOREACH(const KeyEvent& ke, sequence) {
      if (!engine_->ProcessKeyEvent(ke)) {
        // direct commit
        engine_->sink()(std::string(1, ke.keycode()));
      }
    }
    pass_thru_ = false;
  }
}

void ChordComposer::ClearChord() {
    pressed_.clear();
    chord_.clear();
    UpdateChord();
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

}  // namespace rime
