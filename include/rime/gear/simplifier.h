//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SIMPLIFIER_H_
#define RIME_SIMPLIFIER_H_

#include <set>
#include <string>
#include <rime/filter.h>

namespace rime {

class Opencc;

class Simplifier : public Filter {
 public:
  explicit Simplifier(const Ticket& ticket);

  virtual void Apply(CandidateList *recruited,
                     CandidateList *candidates);

 protected:
  typedef enum { kTipsNone, kTipsChar, kTipsAll } TipsLevel;

  void Initialize();
  bool Convert(const shared_ptr<Candidate> &original,
               CandidateList *result);

  bool initialized_;
  scoped_ptr<Opencc> opencc_;
  TipsLevel tips_level_;
  std::string option_name_;
  std::string opencc_config_;
  std::set<std::string> excluded_types_;
};

}  // namespace rime

#endif  // RIME_SIMPLIFIER_H_
