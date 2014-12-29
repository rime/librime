//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-11-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_FILTER_H_
#define RIME_REVERSE_LOOKUP_FILTER_H_

#include <rime/common.h>
#include <rime/filter.h>
#include <rime/algo/algebra.h>
#include <rime/gear/filter_commons.h>

namespace rime {

class ReverseLookupDictionary;

class ReverseLookupFilter : public Filter, TagMatching {
 public:
  explicit ReverseLookupFilter(const Ticket& ticket);

  virtual void Apply(CandidateList* recruited,
                     CandidateList* candidates);

  virtual bool AppliesToSegment(Segment* segment) {
    return TagsMatch(segment);
  }

 protected:
  void Initialize();

  bool initialized_ = false;
  unique_ptr<ReverseLookupDictionary> rev_dict_;
  // settings
  bool overwrite_comment_ = false;
  Projection comment_formatter_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_FILTER_H_
