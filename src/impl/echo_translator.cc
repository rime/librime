// vim: set sts=2 sw=2 et:
// encoding: utf-8
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
#include <rime/impl/echo_translator.h>

namespace rime {

class EchoTranslation : public UniqueTranslation {
 public:
  EchoTranslation(const shared_ptr<Candidate> &candidate)
      : UniqueTranslation(candidate) {
  }
  virtual int Compare(shared_ptr<Translation> other,
                      const CandidateList &candidates) {
    if (!candidates.empty()) {
      set_exhausted(true);
    }
    return UniqueTranslation::Compare(other, candidates);
  }
};

Translation* EchoTranslator::Query(const std::string &input,
                                   const Segment &segment) {
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);
  shared_ptr<Candidate> candidate(
      new Candidate("raw", input, "", segment.start, segment.end, 0));
  Translation *translation = new EchoTranslation(candidate);
  return translation;
}

}  // namespace rime
