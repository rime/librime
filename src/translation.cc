//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-05-21 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/translation.h>

namespace rime {

int Translation::Compare(shared_ptr<Translation> other,
                         const CandidateList& candidates) {
  if (!other || other->exhausted())
    return -1;
  if (exhausted())
    return 1;
  auto ours = Peek();
  auto theirs = other->Peek();
  if (!ours || !theirs)
    return 1;
  int k = 0;
  // the one nearer to the beginning of segment comes first
  k = ours->start() - theirs->start();
  if (k != 0)
    return k;
  // then the longer comes first
  k = ours->end() - theirs->end();
  if (k != 0)
    return -k;
  // compare quality
  double qdiff = ours->quality() - theirs->quality();
  if (qdiff != 0.)
    return (qdiff > 0.) ? -1 : 1;
  // draw
  return 0;
}

}  // namespace rime
