// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SIMPLIFIER_H_
#define RIME_SIMPLIFIER_H_

#include <rime/filter.h>

namespace rime {

class Opencc;

class Simplifier : public Filter {
 public:
  explicit Simplifier(Engine *engine);
  ~Simplifier();

  virtual bool Proceed(CandidateList *recruited,
                       CandidateList *candidates);

 protected:
  typedef enum { kTipNone, kTipChar, kTipAll } TipLevel;

  void Initialize();
  bool Convert(const shared_ptr<Candidate> &original,
               CandidateList *result);
  
  bool initialized_;
  scoped_ptr<Opencc> opencc_;
  TipLevel tip_level_;
  std::string option_name_;
  std::string opencc_config_;
};

}  // namespace rime

#endif  // RIME_SIMPLIFIER_H_
