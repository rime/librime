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
  UniquifiedTranslation(a<Translation> translation,
                        CandidateList* candidates)
      : CacheTranslation(translation), candidates_(candidates) {
    Uniquify();
  }
  virtual bool Next();

 protected:
  bool Uniquify();

  a<Translation> translation_;
  CandidateList* candidates_;
};

bool UniquifiedTranslation::Next() {
  return CacheTranslation::Next() && Uniquify();
}

bool UniquifiedTranslation::Uniquify() {
  if (exhausted()) {
    return false;
  }
  auto next = Peek();
  for (auto& c : *candidates_) {
    if (c->text() == next->text()) {
      auto u = As<UniquifiedCandidate>(c);
      if (!u) {
        u = New<UniquifiedCandidate>(c, "uniquified");
        c = u;
      }
      u->Append(next);
      return CacheTranslation::Next();
    }
  }
  return true;
}

// Uniquifier

Uniquifier::Uniquifier(const Ticket& ticket) : Filter(ticket) {
}

a<Translation> Uniquifier::Apply(a<Translation> translation,
                                          CandidateList* candidates) {
  return New<UniquifiedTranslation>(translation, candidates);
}

}  // namespace rime
