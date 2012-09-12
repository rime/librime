// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-20 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ABC_SEGMENTOR_H_
#define RIME_ABC_SEGMENTOR_H_

#include <set>
#include <string>
#include <rime/segmentor.h>

namespace rime {

class AbcSegmentor : public Segmentor {
 public:
  explicit AbcSegmentor(Engine *engine);

  virtual bool Proceed(Segmentation *segmentation);

 protected:
  std::string alphabet_;
  std::string delimiter_;
  std::set<std::string> extra_tags_;
};

}  // namespace rime

#endif  // RIME_ABC_SEGMENTOR_H_
