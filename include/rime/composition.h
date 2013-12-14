//
// Copyleft RIME Developers
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
  size_t caret_pos = 0;
  size_t sel_start = 0;
  size_t sel_end = 0;
};

class Composition : public Segmentation {
 public:
  Composition() = default;

  bool HasFinishedComposition() const;
  void GetPreedit(Preedit* preedit) const;
  std::string GetCommitText() const;
  std::string GetScriptText() const;
  std::string GetDebugText() const;
};

}  // namespace rime

#endif  // RIME_COMPOSITION_H_
