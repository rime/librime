// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/impl/abc_segmentor.h>

static const char kRimeAlphabet[] = "zyxwvutsrqponmlkjihgfedcba";

namespace rime {

AbcSegmentor::AbcSegmentor(Engine *engine)
    : Segmentor(engine), alphabet_(kRimeAlphabet) {
  // read schema settings
  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/alphabet", &alphabet_);
    config->GetString("speller/delimiter", &delimiter_);
    ConfigListPtr extra_tags = config->GetList("abc_segmentor/extra_tags");
    if (extra_tags) {
      for (size_t i = 0; i < extra_tags->size(); ++i) {
        ConfigValuePtr value = As<ConfigValue>(extra_tags->GetAt(i));
        if (value)
          extra_tags_.insert(value->str());
      }
    }
  }
}

bool AbcSegmentor::Proceed(Segmentation *segmentation) {
  const std::string &input(segmentation->input());
  EZDBGONLYLOGGERVAR(input);
  size_t j = segmentation->GetCurrentStartPosition();
  size_t k = j;
  for (; k < input.length(); ++k) {
    bool is_letter = alphabet_.find(input[k]) != std::string::npos;
    bool is_delimiter =
        (k != j) && (delimiter_.find(input[k]) != std::string::npos);
    if (!is_letter && !is_delimiter)
      break;
  }
  EZDBGONLYLOGGER(j, k);
  if (j < k) {
    Segment segment;
    segment.start = j;
    segment.end = k;
    segment.tags.insert("abc");
    BOOST_FOREACH(const std::string& tag, extra_tags_) {
      segment.tags.insert(tag);
    }
    segmentation->AddSegment(segment);
  }
  // continue this round
  return true;
}

}  // namespace rime
