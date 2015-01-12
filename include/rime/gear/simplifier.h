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

  virtual shared_ptr<Translation> Apply(shared_ptr<Translation> translation,
                                        CandidateList* candidates);


  virtual bool AppliesToSegment(Segment* segment) {
    return TagsMatch(segment);
  }

  bool Convert(const shared_ptr<Candidate>& original,
               CandidateQueue* result);

 protected:
  enum TipsLevel { kTipsNone, kTipsChar, kTipsAll };

  void Initialize();

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
