//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-10-28 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_AFFIX_SEGMENTOR_H_
#define RIME_AFFIX_SEGMENTOR_H_

#include <string>
#include <rime/segmentor.h>

namespace rime {

class AffixSegmentor : public Segmentor {
 public:
  explicit AffixSegmentor(const Ticket& ticket);

  virtual bool Proceed(Segmentation *segmentation);

 protected:
  std::string tag_;
  std::string prefix_;
  std::string suffix_;
  std::string tips_;
  std::string closing_tips_;
};

}  // namespace rime

#endif  // RIME_AFFIX_SEGMENTOR_H_
