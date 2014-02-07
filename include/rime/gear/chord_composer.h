//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-06-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CHORD_COMPOSER_H_
#define RIME_CHORD_COMPOSER_H_

#include <set>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>
#include <rime/algo/algebra.h>

namespace rime {

class ChordComposer : public Processor {
 public:
  ChordComposer(const Ticket& ticket);

  virtual ProcessResult ProcessKeyEvent(const KeyEvent &key_event);

 protected:
  std::string SerializeChord();
  void UpdateChord();
  void FinishChord();
  void ClearChord();
  bool DeleteLastSyllable();
  void OnContextUpdate(Context* ctx);

  std::string alphabet_;
  std::string delimiter_;
  Projection algebra_;
  Projection output_format_;
  Projection prompt_format_;

  std::set<char> pressed_;
  std::set<char> chord_;
  bool pass_thru_;
  bool composing_;
  std::string sequence_;
  connection connection_;
};

}  // namespace rime

#endif  // RIME_CHORD_COMPOSER_H_
