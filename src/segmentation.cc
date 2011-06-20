// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <iterator>
#include <boost/foreach.hpp>
#include <rime/segmentation.h>

namespace rime {

Segmentation::Segmentation(const std::string &input)
    : input_(input), cursor_(0) {}

bool Segmentation::Add(const Segment &segment) {
  int start = 0;
  if (cursor_ > 0) {
    start = segments_[cursor_ - 1].end;
  }
  if (segment.start != start) {
    // rule one: in one round, we examine only those segs
    // that are left-aligned to a same position
    return false;
  }

  if (cursor_ == segments_.size()) {
    // we have a first candidate in this round
    segments_.push_back(segment);
    return true;
  }

  Segment &last = segments_.back();
  if (last.end > segment.end) {
    // rule two: always keep the longer candidate...
  }
  else if (last.end < segment.end) {
    // ...and throw away the shorter seg
    last = segment;
  }
  else {
    // rule three: for equal segments, merge their tags
    std::set<std::string> result;
    std::set_union(last.tags.begin(), last.tags.end(),
                   segment.tags.begin(), segment.tags.end(),
                   std::inserter(result, result.begin()));
    last.tags.swap(result);
  }
  return true;
}

// finalize a round
bool Segmentation::Forward() {
  if (cursor_ >= segments_.size()) {
    return false;
  }
  ++cursor_;
  return true;
}

bool Segmentation::HasFinished() const {
  return (segments_.empty() ? 0 : segments_.back().end) == input_.length();
}

int Segmentation::GetCurrentPosition() const {
  if (cursor_ == 0)
    return 0;
  return segments_[cursor_ - 1].end;
}

int Segmentation::GetCurrentSegmentLength() const {
  if (cursor_ == segments_.size())
    return 0;
  return segments_[cursor_].end - segments_[cursor_].start;
}

std::ostream& operator<< (std::ostream& out, 
                          const Segmentation &segmentation) {
  out << "<" << segmentation.input();
  BOOST_FOREACH(const Segment &segment, segmentation.segments()) {
    out << "|" << segment.start << "," << segment.end;
  }
  out << ">";
  return out;
}

}  // namespace rime
