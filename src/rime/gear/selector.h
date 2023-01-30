//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-09-11 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SELECTOR_H_
#define RIME_SELECTOR_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>
#include <rime/gear/key_binding_processor.h>

namespace rime {

class Selector : public Processor, public KeyBindingProcessor<Selector, 4> {
 public:
  enum TextOrientation {
    Horizontal = 0,
    Vertical = 1,
  };
  enum CandidateListLayout {
    Stacked = 0,
    Linear = 2,
  };

  explicit Selector(const Ticket& ticket);

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;

  Handler PreviousCandidate;
  Handler NextCandidate;
  Handler PreviousPage;
  Handler NextPage;
  Handler Home;
  Handler End;

  bool SelectCandidateAt(Context* ctx, int index);
};

}  // namespace rime

#endif  // RIME_SELECTOR_H_
