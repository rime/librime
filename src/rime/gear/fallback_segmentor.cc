//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-06-20 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/segmentation.h>
#include <rime/gear/fallback_segmentor.h>

namespace rime {

FallbackSegmentor::FallbackSegmentor(const Ticket& ticket)
    : Segmentor(ticket) {
}

bool FallbackSegmentor::Proceed(Segmentation* segmentation) {
  int len = segmentation->GetCurrentSegmentLength();
  DLOG(INFO) << "current segment length: " << len;
  if (len > 0)
    return false;

  const string& input(segmentation->input());
  int k = segmentation->GetCurrentStartPosition();
  DLOG(INFO) << "current start pos: " << k;
  if (k == input.length())
    return false;

  DLOG(INFO) << "segmentation: " << *segmentation;
  if (!segmentation->empty() &&
      segmentation->back().start == segmentation->back().end) {
    segmentation->pop_back();
  }

  if (!segmentation->empty()) {
    Segment& last(segmentation->back());
    // append one character to the last raw segment
    if (last.HasTag("raw")) {
      last.end = k + 1;
      DLOG(INFO) << "extend previous raw segment to ["
                 << last.start << ", " << last.end << ")";
      // mark redo translation (in case it's been previously translated)
      last.Clear();
      last.tags.insert("raw");
      return false;
    }
  }
  {
    Segment segment(k, k + 1);
    DLOG(INFO) << "add a raw segment ["
               << segment.start << ", " << segment.end << ")";
    segment.tags.insert("raw");
    segmentation->Forward();
    segmentation->AddSegment(segment);
  }
  // fallback segmentor should be the last being called, so end this round
  return false;
}

}  // namespace rime
