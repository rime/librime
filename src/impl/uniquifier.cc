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
#include <rime/impl/uniquifier.h>

namespace rime {

// Uniquifier

Uniquifier::Uniquifier(Engine *engine) : Filter(engine) {
}

bool Uniquifier::Proceed(CandidateList *recruited,
                         CandidateList *candidates) {
  if (!candidates || candidates->empty())
    return true;
  CandidateList::iterator i = candidates->begin();
  while (i != candidates->end()) {
    CandidateList::iterator j = recruited->begin();
    for (; j != recruited->end(); ++j) {
      if ((*i)->text() == (*j)->text()) {
        shared_ptr<UniquifiedCandidate> c = As<UniquifiedCandidate>(*j);
        if (!c) {
          c.reset(new UniquifiedCandidate(*j, "uniquified"));
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
