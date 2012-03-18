// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/impl/abc_segmentor.h>

static const char kRimeAlphabet[] = "zyxwvutsrqponmlkjihgfedcba";

namespace rime {

AbcSegmentor::AbcSegmentor(Engine *engine)
    : Segmentor(engine), alphabet_(kRimeAlphabet), max_code_length_(0) {
  // read schema settings
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiter_);
    config->GetInt("speller/max_code_length", &max_code_length_);
  }
}

bool AbcSegmentor::Proceed(Segmentation *segmentation) {
  const std::string &input(segmentation->input());
  EZDBGONLYLOGGERVAR(input);
  size_t j = segmentation->GetCurrentStartPosition();
  size_t k = j;
  bool has_delimiter = false;
  for (; k < input.length(); ++k) {
    bool is_delimiter =
        (k != j) && (delimiter_.find(input[k]) != std::string::npos);
    if (is_delimiter) {
      has_delimiter = true;
      continue;
    }
    bool is_letter = alphabet_.find(input[k]) != std::string::npos;
    if (!is_letter)
      break;
    if (max_code_length_ > 0 && (k - j >= max_code_length_ || has_delimiter))
      break;
  }
  EZDBGONLYLOGGER(j, k);
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
