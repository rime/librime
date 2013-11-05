//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-11-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_FILTER_H_
#define RIME_REVERSE_LOOKUP_FILTER_H_

#include <rime/common.h>
#include <rime/filter.h>
#include <rime/algo/algebra.h>

namespace rime {

class ReverseLookupDictionary;

class ReverseLookupFilter : public Filter {
 public:
  explicit ReverseLookupFilter(const Ticket& ticket);

  virtual void Apply(CandidateList *recruited,
                     CandidateList *candidates);

 protected:
  void Initialize();

  bool initialized_;
  scoped_ptr<ReverseLookupDictionary> rev_dict_;
  bool overwrite_comment_;
  Projection comment_formatter_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_FILTER_H_
