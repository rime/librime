// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-06-19 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/menu.h>

namespace rime {

Composition::Composition() {
}

bool Composition::HasFinishedComposition() const {
  if (empty()) return false;
  size_t k = size() - 1;
  if (k > 0 && at(k).start == at(k).end)
    --k;
  return at(k).status >= Segment::kSelected;
}

void Composition::GetPreedit(Preedit *preedit) const {
  if (!preedit)
    return;
  preedit->text.clear();
  preedit->cursor_pos = 0;
  preedit->sel_start = preedit->sel_end = 0;
  if (empty())
    return;
  size_t text_len = 0;
  size_t start = 0;
  size_t end = 0;
  for (size_t i = 0; i < size(); ++i) {
    start = end;
    const shared_ptr<Candidate> cand(at(i).GetSelectedCandidate());
    if (!cand)
      continue;
    end = cand->end();
    if (i < size() - 1) {
      preedit->text += cand->text();
      text_len = preedit->text.length();
    }
  }
  if (input_.length() > start)
    preedit->text += input_.substr(start);
  preedit->sel_start = text_len;
  preedit->sel_end = text_len + (end - start);
  preedit->cursor_pos = text_len + (back().end - start);
}

const std::string Composition::GetCommitText() const {
  std::string result;
  BOOST_FOREACH(const Segment &seg, *this) {
    const shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
    if (cand)
      result += cand->text();
  }
  return result;
}

const std::string Composition::GetDebugText() const {
  std::string result;
  int i = 0;
  BOOST_FOREACH(const Segment &seg, *this) {
    if (i++ > 0)
      result += "|";
    const shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
    if (cand)
      result += cand->text();
  }
  return result;
}

}  // namespace rime
