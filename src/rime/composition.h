//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-06-19 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMPOSITION_H_
#define RIME_COMPOSITION_H_

#include <rime/segmentation.h>
#include <rime/algo/algebra.h>

namespace rime {

struct Preedit {
  string text;
  size_t caret_pos = 0;
  size_t sel_start = 0;
  size_t sel_end = 0;
};

class Composition : public Segmentation {
 public:
  Composition() = default;

  bool HasFinishedComposition() const;
  Preedit GetPreedit(const string& full_input,
                     size_t caret_pos,
                     const string& caret) const;
  string GetPrompt() const;
  string GetCommitText() const;
  string GetScriptText() const;
  string GetDebugText() const;
  // Returns text of the last segment before the given position.
  string GetTextBefore(size_t pos) const;
  void set_preedit_format(an<ConfigList> patterns){ preedit_formatter_.Load(patterns); }
  Projection preedit_formatter() { return preedit_formatter_; }
protected:
  Projection preedit_formatter_;
};

}  // namespace rime

#endif  // RIME_COMPOSITION_H_
