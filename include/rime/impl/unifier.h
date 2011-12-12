// vim: set sts=2 sw=2 et:
// encoding: utf-8
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

class Unifier : public Filter {
 public:
  explicit Unifier(Engine *engine);

  virtual bool Proceed(CandidateList *recruited,
                       CandidateList *candidates);
};

}  // namespace rime

#endif  // RIME_UNIFIER_H_
