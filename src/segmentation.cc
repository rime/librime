//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <iterator>
#include <ostream>
#include <rime/menu.h>
#include <rime/segmentation.h>

namespace rime {

static const char* kPartialSelectionTag = "partial";

void Segment::Close() {
  auto cand = GetSelectedCandidate();
  if (cand && cand->end() < end) {
    // having selected a partially matched candidate, split it into 2 segments
    end = cand->end();
    tags.insert(kPartialSelectionTag);
  }
}

bool Segment::Reopen(size_t caret_pos) {
  if (status < kSelected) {
    return false;
  }
  const size_t original_end_pos = start + length;
  if (original_end_pos == caret_pos) {
    // reuse previous candidates and keep selection
    if (end < original_end_pos) {
      // restore partial-selected segment
      end = original_end_pos;
      tags.erase(kPartialSelectionTag);
    }
    status = kGuess;
  }
  else {
    status = kVoid;
  }
  return true;
}

an<Candidate> Segment::GetCandidateAt(size_t index) const {
  if (!menu)
    return nullptr;
  return menu->GetCandidateAt(index);
}

an<Candidate> Segment::GetSelectedCandidate() const {
  return GetCandidateAt(selected_index);
}

Segmentation::Segmentation() {
}

void Segmentation::Reset(const string& new_input) {
  DLOG(INFO) << "reset to " << size() << " segments.";
  // mark redo segmentation, while keeping user confirmed segments
  size_t diff_pos = 0;
  while (diff_pos < input_.length() &&
         diff_pos < new_input.length() &&
         input_[diff_pos] == new_input[diff_pos])
    ++diff_pos;
  DLOG(INFO) << "diff pos: " << diff_pos;

  // dispose segments that have changed
  int disposed = 0;
  while (!empty() && back().end > diff_pos) {
    pop_back();
    ++disposed;
  }
  if (disposed > 0)
    Forward();

  input_ = new_input;
}

void Segmentation::Reset(size_t num_segments) {
  if (num_segments >= size())
    return;
  resize(num_segments);
}

bool Segmentation::AddSegment(Segment segment) {
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

  Segment& last = back();
  if (last.end > segment.end) {
    // rule two: always prefer the longer segment...
  }
  else if (last.end < segment.end) {
    // ...and overwrite the shorter one
    last = segment;
  }
  else {
    // rule three: with segments equal in length, merge their tags
    set<string> result;
    set_union(last.tags.begin(), last.tags.end(),
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

// remove empty trailing segment
bool Segmentation::Trim() {
  if (!empty() && back().start == back().end) {
    pop_back();
    return true;
  }
  return false;
}

bool Segmentation::HasFinishedSegmentation() const {
  return (empty() ? 0 : back().end) >= input_.length();
}

size_t Segmentation::GetCurrentStartPosition() const {
  return empty() ? 0 : back().start;
}

size_t Segmentation::GetCurrentEndPosition() const {
  return empty() ? 0 : back().end;
}

size_t Segmentation::GetCurrentSegmentLength() const {
  return empty() ? 0 : (back().end - back().start);
}

size_t Segmentation::GetConfirmedPosition() const {
  size_t k = 0;
  for (const Segment& seg : *this) {
    if (seg.status >= Segment::kSelected)
      k = seg.end;
  }
  return k;
}

std::ostream& operator<< (std::ostream& out,
                          const Segmentation& segmentation) {
  out << "[" << segmentation.input();
  for (const Segment& segment : segmentation) {
    out << "|" << segment.start << "," << segment.end;
    if (!segment.tags.empty()) {
      out << "{";
      bool first = true;
      for (const string& tag : segment.tags) {
        if (first)
          first = false;
        else
          out << ",";
        out << tag;
      }
      out << "}";
    }
  }
  out << "]";
  return out;
}

}  // namespace rime
