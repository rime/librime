//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/gear/uniquifier.h>

namespace rime {

// Uniquifier

Uniquifier::Uniquifier(const Ticket& ticket) : Filter(ticket) {
}

void Uniquifier::Apply(CandidateList* recruited,
                       CandidateList* candidates) {
  if (!candidates || candidates->empty())
    return;
  auto i = candidates->begin();
  while (i != candidates->end()) {
    auto j = recruited->begin();
    for ( ; j != recruited->end(); ++j) {
      if ((*i)->text() == (*j)->text()) {
        auto u = As<UniquifiedCandidate>(*j);
        if (!u) {
          u = make_shared<UniquifiedCandidate>(*j, "uniquified");
          *j = u;
        }
        u->Append(*i);
        break;
      }
    }
    if (j == recruited->end())
      ++i;
    else
      i = candidates->erase(i);
  }
}

}  // namespace rime
