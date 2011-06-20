// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-06-20 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/segmentation.h>
#include "fallback_segmentor.h"

namespace rime {

FallbackSegmentor::FallbackSegmentor(Engine *engine) : Segmentor(engine) {
}

bool FallbackSegmentor::Proceed(Segmentation *segmentation) {
  int len = segmentation->GetCurrentSegmentLength();
  if (len > 0)
    return false;

  const std::string &input = segmentation->input();
  int k = segmentation->GetCurrentPosition();
  if (k == input.length())
    return false;
  
  if (k > 0) {
    Segment &last = segmentation->segments().back();
    // append one character to the last raw segment
    if (last.tags.find("raw") != last.tags.end()) {
      last.end = k + 1;
      return false;
    }
  }
  // add a raw segment
  {
    Segment segment;
    segment.start = k;
    segment.end = k + 1;
    segment.tags.insert("raw");
    segmentation->Add(segment);
  }
  // fallback segmentor should be the last being called, so end this round
  return false;
}  
    
}  // namespace rime
