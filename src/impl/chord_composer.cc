// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-06-05 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>
#include <rime/impl/chord_composer.h>

static const char* kZeroWidthSpace = "\xe2\x80\x8b";

namespace rime {


ChordComposer::ChordComposer(Engine *engine) : Processor(engine) {
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/alphabet", &alphabet_);
  }
}

Processor::Result ChordComposer::ProcessKeyEvent(const KeyEvent &key_event) {
  if (key_event.shift() || key_event.ctrl() || key_event.alt()) {
    pressed_.clear();
    chord_.clear();
    return kNoop;
  }
  bool is_key_up = key_event.release();
  int ch = key_event.keycode();
  if (alphabet_.find(ch) == std::string::npos) {
    pressed_.clear();
    chord_.clear();
    return kNoop;
  }
  if (is_key_up) {
    if (pressed_.erase(ch) != 0) {
      if (pressed_.empty()) {
        FinishChord();
      }
      return kAccepted;
    }
  }
  else {  // key down
    pressed_.insert(ch);
    bool updated = chord_.insert(ch).second;
    if (updated)
      UpdateChord();
    return kAccepted;
  }
  return kNoop;
}

const std::string ChordComposer::SerializeChord() const {
  std::string code;
  for (std::string::const_iterator it = alphabet_.begin();
       it != alphabet_.end(); ++it) {
    if (chord_.find(*it) != chord_.end())
      code.push_back(*it);
  }
  return code;
}

void ChordComposer::UpdateChord() {
  if (!engine_) return;
  Context* ctx = engine_->context();
  Composition* comp = ctx->composition();
  bool chord_exists = !comp->empty() && comp->back().HasTag("chord");
  std::string code(SerializeChord());
  if (code.empty()) {
    if (chord_exists) {
      ctx->ClearPreviousSegment();
    }
  }
  else {
    if (!chord_exists) {
      comp->Forward();
      ctx->PushInput(kZeroWidthSpace);
      comp->back().tags.insert("chord");
    }
    comp->back().prompt = code;
  }
}

void ChordComposer::FinishChord() {
  if (!engine_) return;
  std::string code(SerializeChord());
  chord_.clear();
  UpdateChord();  // clear prompt
  engine_->context()->PushInput(code);
}

}  // namespace rime
