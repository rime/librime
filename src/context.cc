// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-08 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/composition.h>
#include <rime/menu.h>
#include <rime/segmentation.h>

namespace rime {

Context::Context() : composition_(new Composition) {
}

Context::~Context() {
}

bool Context::Commit() {
  if (composition_->empty())
    return false;
  commit_notifier_(this);
  Clear();
  return true;
}

const std::string Context::GetCommitText() const {
  return composition_->GetCommitText();
}

void Context::GetPreedit(Preedit *preedit) const {
  composition_->GetPreedit(preedit);
  preedit->caret_pos = preedit->text.length();
  // TODO: use schema settings
  const std::string caret("\xe2\x80\xba");
  preedit->text.append(caret);
  if (caret_pos_ < input_.length())
    preedit->text.append(input_.substr(caret_pos_));
}

bool Context::IsComposing() const {
  return !input_.empty();
}

bool Context::HasMenu() const {
  return !composition_->empty() && composition_->back().menu;
}

bool Context::PushInput(char ch) {
  if (caret_pos_ >= input_.length()) {
    input_.push_back(ch);
    caret_pos_ = input_.length();
  }
  else {
    input_.insert(caret_pos_, 1, ch);
    ++caret_pos_;
  }
  update_notifier_(this);
  return true;
}

bool Context::PopInput() {
  if (caret_pos_ == 0)
    return false;
  --caret_pos_;
  input_.erase(caret_pos_, 1);
  update_notifier_(this);
  return true;
}

bool Context::DeleteInput() {
  if (caret_pos_ >= input_.length())
    return false;
  input_.erase(caret_pos_, 1);
  update_notifier_(this);
  return true;
}

void Context::Clear() {
  input_.clear();
  update_notifier_(this);
}

bool Context::Select(size_t index) {
  if (composition_->empty())
    return false;
  Segment &seg(composition_->back());
  shared_ptr<Candidate> cand(seg.GetCandidateAt(index));
  if (cand) {
    seg.selected_index = index;
    seg.status = Segment::kSelected;
    EZLOGGERPRINT("Selected: '%s', index = %d.",
                  cand->text().c_str(), index);
    select_notifier_(this);
    return true;
  }
  return false;
}

bool Context::ConfirmCurrentSelection() {
  if (composition_->empty())
    return false;
  Segment &seg(composition_->back());
  shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
  if (cand) {
    seg.status = Segment::kSelected;
    EZLOGGERPRINT("Confirmed: '%s', selected_index = %d.",
                  cand->text().c_str(), seg.selected_index);
    select_notifier_(this);
    return true;
  }
  return false;
}

bool Context::ConfirmPreviousSelection() {
  for (Composition::reverse_iterator it = composition_->rbegin();
       it != composition_->rend(); ++it) {
    if (it->status > Segment::kSelected) return false;
    if (it->status == Segment::kSelected) {
      it->status = Segment::kConfirmed;
      return true;
    }
  }
  return false;
}

bool Context::ReopenPreviousSegment() {
  if (composition_->empty())
    return false;
  Segment &last(composition_->back());
  if (last.start == last.end) {
    composition_->pop_back();
    if (!composition_->empty()) {
      composition_->back().status = Segment::kVoid;
    }
    update_notifier_(this);
    return true;
  }
  return false;
}

bool Context::ReopenPreviousSelection() {
  for (Composition::reverse_iterator it = composition_->rbegin();
       it != composition_->rend(); ++it) {
    if (it->status > Segment::kSelected) return false;
    if (it->status == Segment::kSelected) {
      it->status = Segment::kVoid;
      while (it != composition_->rbegin())
        composition_->pop_back();
      update_notifier_(this);
      return true;
    }
  }
  return false;
}

void Context::set_caret_pos(size_t caret_pos) {
  if (caret_pos > input_.length())
    caret_pos_ = input_.length();
  else
    caret_pos_ = caret_pos;
  update_notifier_(this);
}

void Context::set_composition(Composition *comp) {
  if (composition_.get() != comp)
    composition_.reset(comp);
  // TODO: notification
}

void Context::set_input(const std::string &value) {
  input_ = value;
  caret_pos_ = input_.length();
  update_notifier_(this);
}

Composition* Context::composition() {
  return composition_.get();
}

const Composition* Context::composition() const {
  return composition_.get();
}

}  // namespace rime
