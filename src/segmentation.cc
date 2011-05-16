// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <rime/segmentation.h>

namespace rime {

Segmentation::Segmentation(const std::string &input)
    : input_(input), cursor_(0) {}

bool Segmentation::Add(const Segment &segment) {
  // TODO:
  return false;
}

bool Segmentation::Forward() {
  if (cursor_ >= segments_.size())
    return false;
  ++cursor_;
  return true;
}

bool Segmentation::HasFinished() const {
  return !segments_.empty() && segments_.back().end == input_.length();
}

}  // namespace rime
