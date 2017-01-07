//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-28 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_AFFIX_SEGMENTOR_H_
#define RIME_AFFIX_SEGMENTOR_H_

#include <rime/segmentor.h>

namespace rime {

class AffixSegmentor : public Segmentor {
 public:
  explicit AffixSegmentor(const Ticket& ticket);

  virtual bool Proceed(Segmentation* segmentation);

 protected:
  string tag_;
  string prefix_;
  string suffix_;
  string tips_;
  string closing_tips_;
  set<string> extra_tags_;
};

}  // namespace rime

#endif  // RIME_AFFIX_SEGMENTOR_H_
