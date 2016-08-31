//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-06-20 GONG Chen <chen.sst@gmail.com>
//

#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/gear/echo_translator.h>

namespace rime {

class EchoTranslation : public UniqueTranslation {
 public:
  EchoTranslation(const an<Candidate>& candidate)
      : UniqueTranslation(candidate) {
  }
  virtual int Compare(an<Translation> other,
                      const CandidateList& candidates) {
    if (!candidates.empty() || (other && !other->exhausted())) {
      set_exhausted(true);
    }
    return UniqueTranslation::Compare(other, candidates);
  }
};

EchoTranslator::EchoTranslator(const Ticket& ticket)
    : Translator(ticket) {
}

an<Translation> EchoTranslator::Query(const string& input,
                                      const Segment& segment) {
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";
  auto candidate = New<SimpleCandidate>("raw",
                                        segment.start,
                                        segment.end,
                                        input);
  if (candidate) {
    candidate->set_quality(-100);  // lowest priority
  }
  return New<EchoTranslation>(candidate);
}

}  // namespace rime
