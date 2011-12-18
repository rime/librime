// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ASCII_SEGMENTOR_H_
#define RIME_ASCII_SEGMENTOR_H_

#include <string>
#include <rime/segmentor.h>

namespace rime {

class AsciiSegmentor : public Segmentor {
 public:
  explicit AsciiSegmentor(Engine *engine);

  virtual bool Proceed(Segmentation *segmentation);
};

}  // namespace rime

#endif  // RIME_ASCII_SEGMENTOR_H_
