//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-11-19 Chen Gong <chen.sst@gmail.com>
//
#ifndef RIME_SINGLE_CHAR_FILTER_H_
#define RIME_SINGLE_CHAR_FILTER_H_

#include <rime/filter.h>

namespace rime {

class SingleCharFilter : public Filter {
 public:
  explicit SingleCharFilter(const Ticket& ticket);

  virtual an<Translation> Apply(an<Translation> translation,
                                        CandidateList* candidates);
};

}  // namespace rime

#endif  // RIME_SINGLE_CHAR_FILTER_H_
