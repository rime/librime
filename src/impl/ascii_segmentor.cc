// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/impl/ascii_segmentor.h>

namespace rime {

AsciiSegmentor::AsciiSegmentor(Engine *engine) : Segmentor(engine) {
}

bool AsciiSegmentor::Proceed(Segmentation *segmentation) {
  if (!engine_->context()->get_option("ascii_mode"))
    return true;
  const std::string &input = segmentation->input();
  EZDBGONLYLOGGERVAR(input);
  size_t j = segmentation->GetCurrentStartPosition();
  if (j < input.length()) {
    Segment segment;
    segment.start = j;
    segment.end = input.length();
    segment.tags.insert("raw");
    segmentation->AddSegment(segment);
  }
  return false;  // end of segmentation
}

}  // namespace rime
