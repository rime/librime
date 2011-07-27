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

void Context::Commit() {
  commit_notifier_(this);
  Clear();
}

const std::string Context::GetCommitText() const {
  return composition_->GetText();
}

bool Context::IsComposing() const {
  return !input_.empty();
}

void Context::PushInput(char ch) {
  input_.push_back(ch);
  input_change_notifier_(this);
}

void Context::PopInput() {
  if (input_.empty())
    return;
  input_.resize(input_.size() - 1);
  input_change_notifier_(this);
}

void Context::Clear() {
  input_.clear();
  input_change_notifier_(this);
}

bool Context::ConfirmCurrentSelection() {
  if (composition_->empty())
    return false;
  Segment &seg(composition_->back());
  shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
  if (cand) {
    seg.status = Segment::kConfirmed;
    EZLOGGERPRINT("Confirmed: %s, selected_index = %d.",
                  cand->text().c_str(), seg.selected_index);
    select_notifier_(this);
    return true;
  }
  else {
    return false;
  }
}

void Context::set_composition(Composition *comp) {
  if (composition_.get() != comp)
    composition_.reset(comp);
  // TODO: notification
}

void Context::set_input(const std::string &value) {
  input_ = value;
  input_change_notifier_(this);
}

Composition* Context::composition() {
  return composition_.get();
}

const Composition* Context::composition() const {
  return composition_.get();
}

}  // namespace rime
