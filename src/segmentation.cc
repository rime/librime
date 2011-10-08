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
  if (!menu || index < 0) 
    return shared_ptr<Candidate>();
  else
    return menu->GetCandidateAt(index);
}

const shared_ptr<Candidate> Segment::GetSelectedCandidate() const {
  return GetCandidateAt(selected_index);
}

Segmentation::Segmentation() {
}

void Segmentation::Reset(const std::string &new_input) {
  EZLOGGERVAR(size());
  // mark redo segmentation, while keeping user confirmed segments
  size_t diff_pos = 0;
  while (diff_pos < input_.length() &&
         diff_pos < new_input.length() &&
         input_[diff_pos] == new_input[diff_pos])
    ++diff_pos;
  EZLOGGERVAR(diff_pos);

  // dispose segments that have changed
  int disposed = 0;
  while (!empty() && back().end > diff_pos) {
    pop_back();
    ++disposed;
  }
  if (disposed > 0) Forward();
  
  input_ = new_input;
}

void Segmentation::Reset(int num_segments) {
  if (num_segments >= size())
    return;
  resize(num_segments);
}

bool Segmentation::AddSegment(const Segment &segment) {
  int start = GetCurrentStartPosition();
  if (segment.start != start) {
    // rule one: in one round, we examine only those segs
    // that are left-aligned to a same position
    return false;
  }

  if (empty()) {
    push_back(segment);
    return true;
  }

  Segment &last = back();
  if (last.end > segment.end) {
    // rule two: always prefer the longer segment...
  }
  else if (last.end < segment.end) {
    // ...and overwrite the shorter one
    last = segment;
  }
  else {
    // rule three: with segments equal in length, merge their tags
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
  if (empty() || back().start == back().end)
    return false;
  // initialize an empty segment for the next round
  push_back(Segment(back().end, back().end));
  return true;
}

bool Segmentation::HasFinished() const {
  return (empty() ? 0 : back().end) == input_.length();
}

int Segmentation::GetCurrentStartPosition() const {
  return empty() ? 0 : back().start;
}

int Segmentation::GetCurrentEndPosition() const {
  return empty() ? 0 : back().end;
}

int Segmentation::GetCurrentSegmentLength() const {
  return empty() ? 0 : (back().end - back().start);
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
