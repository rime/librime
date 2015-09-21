//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UNIFIER_H_
#define RIME_UNIFIER_H_

#include <rime/filter.h>

namespace rime {

class Uniquifier : public Filter {
 public:
  explicit Uniquifier(const Ticket& ticket);

  virtual an<Translation> Apply(an<Translation> translation,
                                        CandidateList* candidates);

};

}  // namespace rime

#endif  // RIME_UNIFIER_H_
