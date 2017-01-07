//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-01 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/gear/matcher.h>

namespace rime {

Matcher::Matcher(const Ticket& ticket) : Segmentor(ticket) {
  // read schema settings
  if (!ticket.schema)
    return;
  Config* config = ticket.schema->config();
  patterns_.LoadConfig(config);
}

bool Matcher::Proceed(Segmentation* segmentation) {
  if (patterns_.empty())
    return true;
  auto match = patterns_.GetMatch(segmentation->input(), *segmentation);
  if (match.found()) {
    DLOG(INFO) << "match: " << match.tag
               << " [" << match.start << ", " << match.end << ")";
    while (segmentation->GetCurrentStartPosition() > match.start)
      segmentation->pop_back();
    Segment segment(match.start, match.end);
    segment.tags.insert(match.tag);
    segmentation->AddSegment(segment);
    // terminate this round?
    //return false;
  }
  return true;
}

}  // namespace rime
