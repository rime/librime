//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-06-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CHORD_COMPOSER_H_
#define RIME_CHORD_COMPOSER_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/key_event.h>
#include <rime/processor.h>
#include <rime/algo/algebra.h>

namespace rime {

class ChordComposer : public Processor {
 public:
  ChordComposer(const Ticket& ticket);
  ~ChordComposer();

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  ProcessResult ProcessChordingKey(const KeyEvent& key_event);
  ProcessResult ProcessFunctionKey(const KeyEvent& key_event);
  string SerializeChord();
  void UpdateChord();
  void FinishChord();
  void ClearChord();
  bool DeleteLastSyllable();
  void OnContextUpdate(Context* ctx);
  void OnUnhandledKey(Context* ctx, const KeyEvent& key);

  KeySequence chording_keys_;
  string delimiter_;
  Projection algebra_;
  Projection output_format_;
  Projection prompt_format_;

  set<int> pressed_;
  set<int> chord_;
  bool pass_thru_ = false;
  bool composing_ = false;
  string raw_sequence_;
  connection update_connection_;
  connection unhandled_key_connection_;
};

}  // namespace rime

#endif  // RIME_CHORD_COMPOSER_H_
