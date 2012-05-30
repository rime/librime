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
  preedit->sel_start = preedit->sel_end = 0;
  if (empty())
    return;
  size_t text_len = 0;
  size_t start = 0;
  size_t end = 0;
  for (size_t i = 0; i < size(); ++i) {
    start = end;
    const shared_ptr<Candidate> cand(at(i).GetSelectedCandidate());
    if (i < size() - 1) {  // converted
      if (cand) {
        end = cand->end();
        preedit->text += cand->text();
        text_len = preedit->text.length();
      }
      else {  // raw input
        end = at(i).end;
        preedit->text += input_.substr(start, end - start);
        text_len = preedit->text.length();
      }
    }
    else {  // highlighted
      preedit->sel_start = text_len;
      if (cand && !cand->preedit().empty()) {
        end = cand->end();
        preedit->text += cand->preedit();
      }
      else {
        end = at(i).end;
        preedit->text += input_.substr(start, end - start);
      }
      if (!at(i).prompt.empty()) {  // show prompt
        preedit->text += at(i).prompt;
      }
      text_len = preedit->text.length();
      preedit->sel_end = text_len;
    }
  }
  if (input_.length() > end) {
    preedit->text += input_.substr(end);
  }
}

const std::string Composition::GetCommitText() const {
  std::string result;
  size_t end = 0;
  BOOST_FOREACH(const Segment &seg, *this) {
    const shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
    if (cand) {
      end = cand->end();
      result += cand->text();
    }
    else {
      end = seg.end;
      result += input_.substr(seg.start, seg.end - seg.start);
    }
  }
  if (input_.length() > end) {
    result += input_.substr(end);
  }
  return result;
}

const std::string Composition::GetScriptText() const {
  std::string result;
  size_t start = 0;
  size_t end = 0;
  BOOST_FOREACH(const Segment &seg, *this) {
    const shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
    start = end;
    end = cand ? cand->end() : seg.end;
    if (cand && !cand->preedit().empty())
      result += cand->preedit();
    else
      result += input_.substr(start, end - start);
  }
  if (input_.length() > end) {
    result += input_.substr(end);
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
    else
      result += input_.substr(seg.start, seg.end - seg.start);
  }
  return result;
}

}  // namespace rime
