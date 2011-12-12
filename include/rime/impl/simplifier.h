// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-20 GONG Chen <chen.sst@gmail.com>
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
  scoped_ptr<Opencc> opencc_;
};

}  // namespace rime

#endif  // RIME_SIMPLIFIER_H_
