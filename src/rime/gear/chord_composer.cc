//
// Copyright RIME Developers
// Distributed under the BSD License
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
#include <rime/gear/chord_composer.h>

namespace rime {

ChordComposer::ChordComposer(const Ticket& ticket) : Processor(ticket) {
  if (!engine_)
    return;
  if (Config* config = engine_->schema()->config()) {
    string alphabet;
    config->GetString("chord_composer/alphabet", &alphabet);
    chording_keys_.Parse(alphabet);
    config->GetBool("chord_composer/use_control", &use_control_);
    config->GetBool("chord_composer/use_alt", &use_alt_);
    config->GetBool("chord_composer/use_shift", &use_shift_);
    config->GetString("speller/delimiter", &delimiter_);
    algebra_.Load(config->GetList("chord_composer/algebra"));
    output_format_.Load(config->GetList("chord_composer/output_format"));
    prompt_format_.Load(config->GetList("chord_composer/prompt_format"));
  }
  Context* ctx = engine_->context();
  ctx->set_option("_chord_typing", true);
  update_connection_ = ctx->update_notifier().connect(
      [this](Context* ctx) { OnContextUpdate(ctx); });
  unhandled_key_connection_ = ctx->unhandled_key_notifier().connect(
      [this](Context* ctx, const KeyEvent& key) { OnUnhandledKey(ctx, key); });
}

ChordComposer::~ChordComposer() {
  update_connection_.disconnect();
  unhandled_key_connection_.disconnect();
}

ProcessResult ChordComposer::ProcessFunctionKey(const KeyEvent& key_event) {
  if (key_event.release()) {
    return kNoop;
  }
  int ch = key_event.keycode();
  if (ch == XK_Return) {
    if (!raw_sequence_.empty()) {
      // commit raw input
      engine_->context()->set_input(raw_sequence_);
      // then the sequence should not be used again
      raw_sequence_.clear();
    }
    ClearChord();
  } else if (ch == XK_BackSpace || ch == XK_Escape) {
    // clear the raw sequence
    raw_sequence_.clear();
    ClearChord();
  }
  return kNoop;
}

// Note: QWERTY layout only.
static const char map_to_base_layer[] = {
    " 1'3457'908=,-./"
    "0123456789;;,=./"
    "2abcdefghijklmno"
    "pqrstuvwxyz[\\]6-"
    "`abcdefghijklmno"
    "pqrstuvwxyz[\\]`"};

inline static int get_base_layer_key_code(const KeyEvent& key_event) {
  int ch = key_event.keycode();
  bool is_shift = key_event.shift();
  return (is_shift && ch >= 0x20 && ch <= 0x7e) ? map_to_base_layer[ch - 0x20]
                                                : ch;
}

ProcessResult ChordComposer::ProcessChordingKey(const KeyEvent& key_event) {
  if (key_event.ctrl() || key_event.alt()) {
    raw_sequence_.clear();
  }
  if ((key_event.ctrl() && !use_control_) || (key_event.alt() && !use_alt_) ||
      (key_event.shift() && !use_shift_)) {
    ClearChord();
    return kNoop;
  }
  int ch = get_base_layer_key_code(key_event);
  // non chording key
  if (std::find(chording_keys_.begin(), chording_keys_.end(),
                KeyEvent{ch, 0}) == chording_keys_.end()) {
    ClearChord();
    return kNoop;
  }
  // chording key
  editing_chord_ = true;
  bool is_key_up = key_event.release();
  if (is_key_up) {
    if (pressed_.erase(ch) != 0 && pressed_.empty()) {
      FinishChord();
    }
  } else {  // key down
    pressed_.insert(ch);
    bool updated = chord_.insert(ch).second;
    if (updated)
      UpdateChord();
  }
  editing_chord_ = false;
  return kAccepted;
}

ProcessResult ChordComposer::ProcessKeyEvent(const KeyEvent& key_event) {
  if (engine_->context()->get_option("ascii_mode")) {
    return kNoop;
  }
  if (sending_chord_) {
    return ProcessFunctionKey(key_event);
  }
  bool is_key_up = key_event.release();
  int ch = key_event.keycode();
  if (!is_key_up && ch >= 0x20 && ch <= 0x7e) {
    // save raw input
    if (!engine_->context()->IsComposing() || !raw_sequence_.empty()) {
      raw_sequence_.push_back(ch);
      DLOG(INFO) << "update raw sequence: " << raw_sequence_;
    }
  }
  auto result = ProcessChordingKey(key_event);
  if (result != kNoop) {
    return result;
  }
  return ProcessFunctionKey(key_event);
}

string ChordComposer::SerializeChord() {
  KeySequence key_sequence;
  for (KeyEvent key : chording_keys_) {
    if (chord_.find(key.keycode()) != chord_.end())
      key_sequence.push_back(key);
  }
  string code = key_sequence.repr();
  algebra_.Apply(&code);
  return code;
}

void ChordComposer::UpdateChord() {
  if (!engine_)
    return;
  Context* ctx = engine_->context();
  Composition& comp = ctx->composition();
  string code = SerializeChord();
  prompt_format_.Apply(&code);
  if (comp.empty()) {
    // add a placeholder segment
    // 1. to cheat ctx->IsComposing() == true
    // 2. to attach chord prompt to while chording
    Segment placeholder(0, ctx->input().length());
    placeholder.tags.insert("phony");
    ctx->composition().AddSegment(placeholder);
  }
  auto& last_segment = comp.back();
  last_segment.tags.insert("chord_prompt");
  last_segment.prompt = code;
}

void ChordComposer::FinishChord() {
  if (!engine_)
    return;
  string code = SerializeChord();
  output_format_.Apply(&code);
  ClearChord();

  KeySequence key_sequence;
  if (key_sequence.Parse(code) && !key_sequence.empty()) {
    sending_chord_ = true;
    for (const KeyEvent& key : key_sequence) {
      if (!engine_->ProcessKey(key)) {
        // direct commit
        engine_->CommitText(string(1, key.keycode()));
        // exclude the character (eg. space) from the raw sequence
        raw_sequence_.clear();
      }
    }
    sending_chord_ = false;
  }
}

void ChordComposer::ClearChord() {
  pressed_.clear();
  chord_.clear();
  if (!engine_)
    return;
  Context* ctx = engine_->context();
  Composition& comp = ctx->composition();
  if (comp.empty())
    return;
  auto& last_segment = comp.back();
  if (comp.size() == 1 && last_segment.HasTag("phony")) {
    ctx->Clear();
  } else if (last_segment.HasTag("chord_prompt")) {
    last_segment.prompt.clear();
    last_segment.tags.erase("chord_prompt");
  }
}

void ChordComposer::OnContextUpdate(Context* ctx) {
  if (ctx->IsComposing()) {
    composing_ = true;
  } else if (composing_) {
    composing_ = false;
    if (!editing_chord_ || sending_chord_) {
      raw_sequence_.clear();
      DLOG(INFO) << "clear raw sequence.";
    }
  }
}

void ChordComposer::OnUnhandledKey(Context* ctx, const KeyEvent& key) {
  // directly committed ascii should not be captured into the raw sequence
  // test case:
  // 3.14{Return} should not commit an extra sequence '14'
  if ((key.modifier() & ~kShiftMask) == 0 && key.keycode() >= 0x20 &&
      key.keycode() <= 0x7e) {
    raw_sequence_.clear();
    DLOG(INFO) << "clear raw sequence.";
  }
}

}  // namespace rime
