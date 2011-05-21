// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <rime/segmentation.h>
#include "abc_segmentor.h"

namespace rime {

AbcSegmentor::AbcSegmentor(Engine *engine) : Segmentor(engine) {
  // TODO: read schema settings
  alphabet_ = "zyxwvutsrqponmlkjihgfedcba";
}

bool AbcSegmentor::Proceed(Segmentation *segmentation) {
  const std::string &input = segmentation->input();
  int k = segmentation->cursor();
  for (; k < input.length(); ++k) {
    if (alphabet_.find(input[k]) == std::string::npos)
      break;
  }
  if (k > segmentation->cursor()) {
    Segment segment;
    segment.start = segmentation->cursor();
    segment.end = k;
    segment.tags.insert("abc");
    segmentation->Add(segment);
  }
  // continue this round
  return true;
}  
    
}  // namespace rime
