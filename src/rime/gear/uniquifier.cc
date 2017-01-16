//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/translation.h>
#include <rime/gear/uniquifier.h>

namespace rime {

class UniquifiedTranslation : public CacheTranslation {
 public:
  UniquifiedTranslation(an<Translation> translation,
                        CandidateList* candidates)
      : CacheTranslation(translation), candidates_(candidates) {
    Uniquify();
  }
  virtual bool Next();

 protected:
  bool Uniquify();

  an<Translation> translation_;
  CandidateList* candidates_;
};

bool UniquifiedTranslation::Next() {
  return CacheTranslation::Next() && Uniquify();
}

static CandidateList::iterator find_text_match(const an<Candidate>& target,
                                               CandidateList::iterator begin,
                                               CandidateList::iterator end) {
  for (auto iter = begin; iter != end; ++iter) {
    if ((*iter)->text() == target->text()) {
      return iter;
    }
  }
  return end;
}

bool UniquifiedTranslation::Uniquify() {
  while (!exhausted()) {
    auto next = Peek();
    CandidateList::iterator previous = find_text_match(
        next, candidates_->begin(), candidates_->end());
    if (previous == candidates_->end()) {
      // Encountered a unique candidate.
      return true;
    }
    auto uniquified = As<UniquifiedCandidate>(*previous);
    if (!uniquified) {
      *previous = uniquified =
          New<UniquifiedCandidate>(*previous, "uniquified");
    }
    uniquified->Append(next);
    CacheTranslation::Next();
  }
  return false;
}

// Uniquifier

Uniquifier::Uniquifier(const Ticket& ticket) : Filter(ticket) {
}

an<Translation> Uniquifier::Apply(an<Translation> translation,
                                          CandidateList* candidates) {
  return New<UniquifiedTranslation>(translation, candidates);
}

}  // namespace rime
