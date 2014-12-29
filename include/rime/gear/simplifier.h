//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SIMPLIFIER_H_
#define RIME_SIMPLIFIER_H_

#include <set>
#include <string>
#include <rime/filter.h>
#include <rime/gear/filter_commons.h>

namespace rime {

class Opencc;

class Simplifier : public Filter, TagMatching {
 public:
  explicit Simplifier(const Ticket& ticket);

  virtual void Apply(CandidateList* recruited,
                     CandidateList* candidates);

  virtual bool AppliesToSegment(Segment* segment) {
    return TagsMatch(segment);
  }

 protected:
  enum TipsLevel { kTipsNone, kTipsChar, kTipsAll };

  void Initialize();
  bool Convert(const shared_ptr<Candidate>& original,
               CandidateList* result);

  bool initialized_ = false;
  unique_ptr<Opencc> opencc_;
  // settings
  TipsLevel tips_level_ =  kTipsNone;
  std::string option_name_;
  std::string opencc_config_;
  std::set<std::string> excluded_types_;
};

}  // namespace rime

#endif  // RIME_SIMPLIFIER_H_
