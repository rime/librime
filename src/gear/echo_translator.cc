//
// Copyleft 2011 RIME Developers
// License: GPLv3
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
  EchoTranslation(const shared_ptr<Candidate> &candidate)
      : UniqueTranslation(candidate) {
  }
  virtual int Compare(shared_ptr<Translation> other,
                      const CandidateList &candidates) {
    if (!candidates.empty() || other && other->Peek()) {
      set_exhausted(true);
      return 1;
    }
    return UniqueTranslation::Compare(other, candidates);
  }
};

EchoTranslator::EchoTranslator(const TranslatorTicket& ticket)
    : Translator(ticket) {
}

shared_ptr<Translation> EchoTranslator::Query(const std::string& input,
                                              const Segment& segment,
                                              std::string* prompt) {
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";
  shared_ptr<Candidate> candidate =
      boost::make_shared<SimpleCandidate>("raw",
                                          segment.start,
                                          segment.end,
                                          input);
  if (candidate) {
    candidate->set_quality(-100);  // lowest priority
  }
  return make_shared<EchoTranslation>(candidate);
}

}  // namespace rime
