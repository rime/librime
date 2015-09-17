//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-06 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>

namespace rime {

static an<Candidate>
UnpackShadowCandidate(const an<Candidate>& cand) {
  auto shadow = As<ShadowCandidate>(cand);
  return shadow ? shadow->item() : cand;
}

an<Candidate>
Candidate::GetGenuineCandidate(const an<Candidate>& cand) {
  auto uniquified = As<UniquifiedCandidate>(cand);
  return UnpackShadowCandidate(uniquified ? uniquified->items().front() : cand);
}

vector<of<Candidate>>
Candidate::GetGenuineCandidates(const an<Candidate>& cand) {
  vector<of<Candidate>> result;
  if (auto uniquified = As<UniquifiedCandidate>(cand)) {
    for (const auto& item : uniquified->items()) {
      result.push_back(UnpackShadowCandidate(item));
    }
  }
  else {
    result.push_back(UnpackShadowCandidate(cand));
  }
  return result;
}

}  // namespace rime
