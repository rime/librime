//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/gear/ascii_segmentor.h>

namespace rime {

AsciiSegmentor::AsciiSegmentor(const Ticket& ticket) : Segmentor(ticket) {
}

bool AsciiSegmentor::Proceed(Segmentation* segmentation) {
  if (!engine_->context()->get_option("ascii_mode"))
    return true;
  const string& input = segmentation->input();
  size_t j = segmentation->GetCurrentStartPosition();
  if (j < input.length()) {
    Segment segment(j, input.length());
    segment.tags.insert("raw");
    segmentation->AddSegment(segment);
  }
  return false;  // end of segmentation
}

}  // namespace rime
