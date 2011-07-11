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

void Context::ConfirmCurrentSelection() {
  if (composition_->empty())
    return;
  Selection &sel(composition_->back());
  shared_ptr<Candidate> cand(sel.menu->GetCandidateAt(sel.index));
  if (cand) {
    sel.manner = Selection::kConfirmed;
    EZLOGGERPRINT("Confirmed: %s, index = %d.",
                  cand->text().c_str(), sel.index);
    // TODO: notification
  }
}

void Context::set_input(const std::string &value) {
  input_ = value;
  input_change_notifier_(this);
}

void Context::set_segmentation(Segmentation *segmentation) {
  segmentation_.reset(segmentation);
}

const Segmentation* Context::segmentation() const {
  return segmentation_.get();
}

Composition* Context::composition() {
  return composition_.get();
}

const Composition* Context::composition() const {
  return composition_.get();
}

}  // namespace rime
