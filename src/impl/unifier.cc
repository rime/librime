// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/engine.h>
#include <rime/impl/unifier.h>

namespace rime {

// Unifier

Unifier::Unifier(Engine *engine) : Filter(engine) {
}

bool Unifier::Proceed(CandidateList *recruited,
                      CandidateList *candidates) {
  if (!candidates || candidates->empty())
    return false;
  CandidateList::iterator i = candidates->begin();
  while (i != candidates->end()) {
    CandidateList::iterator j = recruited->begin();
    for (; j != recruited->end(); ++j) {
      if ((*i)->text() == (*j)->text()) {
        shared_ptr<UnifiedCandidate> c = As<UnifiedCandidate>(*j);
        if (!c) {
          c.reset(new UnifiedCandidate(*j, "unified"));
          *j = c;
        }
        c->Append(*i);
        break;
      }
    }
    if (j == recruited->end())
      ++i;
    else
      i = candidates->erase(i);
  }
  return true;
}

}  // namespace rime
