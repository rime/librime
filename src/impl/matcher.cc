// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-01-01 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/impl/matcher.h>

namespace rime {

Matcher::Matcher(Engine *engine) : Segmentor(engine) {
  // read schema settings
  Config *config = engine->schema()->config();
  if (!config) return;
  patterns_.LoadConfig(config);
}

bool Matcher::Proceed(Segmentation *segmentation) {
  if (patterns_.empty()) return true;
  size_t k = 0;
  BOOST_FOREACH(const Segment &seg, *segmentation) {
    if (seg.status >= Segment::kSelected)
      k = seg.end;
  }
  std::string input(segmentation->input().substr(k));
  BOOST_FOREACH(const RecognizerPatterns::value_type &v, patterns_) {
    if (boost::regex_match(input, v.second)) {
      EZLOGGERPRINT("input[%d,$] '%s' matches pattern: %s",
                    k, input.c_str(), v.first.c_str());
      while (!segmentation->empty() &&
             segmentation->GetCurrentStartPosition() > k) {
        segmentation->pop_back();
      }
      Segment segment;
      segment.start = segmentation->GetCurrentStartPosition();
      segment.end = segmentation->input().length();
      segment.tags.insert(v.first);
      segment.tags.insert("raw");
      segmentation->AddSegment(segment);
      // terminate this round
      return false;
    }
  }
  return true;
}

}  // namespace rime
