// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/segmentation.h>
#include <rime/impl/abc_segmentor.h>

static const char kRimeAlphabet[] = "zyxwvutsrqponmlkjihgfedcba";

namespace rime {

AbcSegmentor::AbcSegmentor(Engine *engine)
    : Segmentor(engine), alphabet_(kRimeAlphabet) {
  // TODO: read schema settings

}

bool AbcSegmentor::Proceed(Segmentation *segmentation) {
  const std::string &input = segmentation->input();
  EZLOGGERVAR(input);
  int k = segmentation->GetCurrentPosition();
  for (; k < input.length(); ++k) {
    if (alphabet_.find(input[k]) == std::string::npos)
      break;
  }
  int j = segmentation->GetCurrentPosition();
  if (j < k) {
    Segment segment;
    segment.start = j;
    segment.end = k;
    segment.tags.insert("abc");
    segmentation->AddSegment(segment);
  }
  // continue this round
  return true;
}  
    
}  // namespace rime
