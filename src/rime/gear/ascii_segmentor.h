//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_ASCII_SEGMENTOR_H_
#define RIME_ASCII_SEGMENTOR_H_

#include <rime/segmentor.h>

namespace rime {

class AsciiSegmentor : public Segmentor {
 public:
  explicit AsciiSegmentor(const Ticket& ticket);

  virtual bool Proceed(Segmentation* segmentation);
};

}  // namespace rime

#endif  // RIME_ASCII_SEGMENTOR_H_
