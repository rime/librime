//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-10-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SPELLER_H_
#define RIME_SPELLER_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Speller : public Processor {
 public:
  Speller(const Ticket& ticket);

  virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

 protected:
  std::string alphabet_;
  std::string delimiters_;
  std::string initials_;
  std::string finals_;
  int max_code_length_ = 0;
  bool auto_select_ = false;
  bool use_space_ = false;
};

}  // namespace rime

#endif  // RIME_SPELLER_H_
