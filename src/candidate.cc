//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-01-06 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>

namespace rime {

const shared_ptr<Candidate>
Candidate::GetGenuineCandidate(const shared_ptr<Candidate>& cand) {
  shared_ptr<Candidate> result(cand);
  if (result) {
    shared_ptr<UniquifiedCandidate> uniquified(As<UniquifiedCandidate>(result));
    if (uniquified)
      result = uniquified->items().front();
    shared_ptr<ShadowCandidate> shadow(As<ShadowCandidate>(result));
    if (shadow)
      result = shadow->item();
  }
  return result;
}

}  // namespace rime
