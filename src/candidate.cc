//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-01-06 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>

namespace rime {

static shared_ptr<Candidate>
UnpackShadowCandidate(shared_ptr<Candidate> cand) {
  shared_ptr<ShadowCandidate> shadow(As<ShadowCandidate>(cand));
  return shadow ? shadow->item() : cand;
}

shared_ptr<Candidate>
Candidate::GetGenuineCandidate(const shared_ptr<Candidate>& cand) {
  shared_ptr<UniquifiedCandidate> uniquified(As<UniquifiedCandidate>(cand));
  return UnpackShadowCandidate(uniquified ? uniquified->items().front() : cand);
}

std::vector<shared_ptr<Candidate> >
Candidate::GetGenuineCandidates(const shared_ptr<Candidate>& cand) {
  std::vector<shared_ptr<Candidate> > result;
  shared_ptr<UniquifiedCandidate> uniquified(As<UniquifiedCandidate>(cand));
  if (uniquified) {
    for (const shared_ptr<Candidate>& item : uniquified->items()) {
      result.push_back(UnpackShadowCandidate(item));
    }
  }
  else {
    result.push_back(UnpackShadowCandidate(cand));
  }
  return result;
}

}  // namespace rime
