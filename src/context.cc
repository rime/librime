// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-08 GONG Chen <chen.sst@gmail.com>
//
#include <rime/context.h>
#include <rime/segmentation.h>

namespace rime {

Context::Context() {
}

Context::~Context() {
}

void Context::Commit() {
  commit_notifier_(this);
  Clear();
}

const std::string Context::GetCommitText() const {
  // TODO: echo...
  return input();
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

}  // namespace rime
