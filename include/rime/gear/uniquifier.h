//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UNIFIER_H_
#define RIME_UNIFIER_H_

#include <rime/filter.h>

namespace rime {

class Uniquifier : public Filter {
 public:
  explicit Uniquifier(Engine *engine);

  virtual void Apply(CandidateList *recruited,
                     CandidateList *candidates);
};

}  // namespace rime

#endif  // RIME_UNIFIER_H_
