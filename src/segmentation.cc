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
#include <rime/menu.h>
#include <rime/segmentation.h>

namespace rime {

const shared_ptr<Candidate> Segment::GetCandidateAt(int index) const {
  if (!menu) 
    return shared_ptr<Candidate>();
  else
    return menu->GetCandidateAt(index);
}

const shared_ptr<Candidate> Segment::GetSelectedCandidate() const {
  return GetCandidateAt(selected_index);
}

Segmentation::Segmentation()
    : input_(), cursor_(0) {}

void Segmentation::Reset(const std::string &input) {
  // mark redo segmentation, while keeping user confirmed segments
  size_t diff_pos = 0;
  while (diff_pos < input_.length() &&
         diff_pos < input.length() &&
         input_[diff_pos] == input[diff_pos])
    ++diff_pos;

  // discard segments that have changed
  while (cursor_ > 0 && at(cursor_ - 1).end > diff_pos)
    --cursor_;
  resize(cursor_);

  // then investigate the last segment if not having been confirmed
  if (cursor_ > 0 && at(cursor_ - 1).status <= Segment::kGuess)
    --cursor_;

  input_ = input;
}

void Segmentation::Reset(int cursor_pos) {
  if (cursor_pos > cursor_)
    return;
  cursor_ = cursor_pos;
  resize(cursor_pos);
}

bool Segmentation::AddSegment(const Segment &segment) {
  int start = 0;
  if (cursor_ > 0) {
    start = at(cursor_ - 1).end;
  }
  if (segment.start != start) {
    // rule one: in one round, we examine only those segs
    // that are left-aligned to a same position
    return false;
  }

  if (cursor_ == size()) {
    // we have a first candidate in this round
    push_back(segment);
    return true;
  }

  Segment &last = back();
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
  if (cursor_ >= size()) {
    return false;
  }
  ++cursor_;
  return true;
}

bool Segmentation::HasFinished() const {
  return (empty() ? 0 : back().end) == input_.length();
}

int Segmentation::GetCurrentPosition() const {
  if (cursor_ == 0)
    return 0;
  return at(cursor_ - 1).end;
}

int Segmentation::GetCurrentSegmentLength() const {
  if (cursor_ == size())
    return 0;
  return at(cursor_).end - at(cursor_).start;
}

std::ostream& operator<< (std::ostream& out,
                          const Segmentation &segmentation) {
  out << "<" << segmentation.input();
  BOOST_FOREACH(const Segment &segment, segmentation) {
    out << "|" << segment.start << "," << segment.end;
  }
  out << ">";
  return out;
}

}  // namespace rime
