// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-06-19 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMPOSITION_H_
#define RIME_COMPOSITION_H_

#include <string>
#include <rime/segmentation.h>

namespace rime {

struct Preedit {
  std::string text;
  int cursor_pos;
  int sel_start;
  int sel_end;
};

class Composition : public Segmentation {
 public:
  Composition();
  void GetPreedit(Preedit *preedit) const;
  const std::string GetCommitText() const;
  const std::string GetDebugText() const;
};

}  // namespace rime

#endif  // RIME_COMPOSITION_H_
