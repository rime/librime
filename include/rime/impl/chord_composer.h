// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
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
  ChordComposer(Engine *engine);

  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
  
 protected:
  const std::string SerializeChord();
  void UpdateChord();
  void FinishChord();
  void ClearChord();
  bool DeleteLastSyllable();
  
  std::string alphabet_;
  std::string delimiter_;
  Projection algebra_;
  Projection output_format_;
  Projection prompt_format_;

  std::set<char> pressed_;
  std::set<char> chord_;
  bool pass_thru_;
};

}  // namespace rime

#endif  // RIME_CHORD_COMPOSER_H_
