// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
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
  Speller(Engine *engine);
  virtual ~Speller() {}
  virtual Result ProcessKeyEvent(const KeyEvent &key_event);
  
 protected:
  std::string alphabet_;
  std::string delimiter_;
  std::string initials_;
};

}  // namespace rime

#endif  // RIME_SPELLER_H_
