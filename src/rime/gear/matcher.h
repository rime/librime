//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-01 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MATCHER_H_
#define RIME_MATCHER_H_

#include <rime/segmentor.h>
#include "recognizer.h"

namespace rime {

class Matcher : public Segmentor {
 public:
  explicit Matcher(const Ticket& ticket);

  virtual bool Proceed(Segmentation* segmentation);

 protected:
  RecognizerPatterns patterns_;
};

}  // namespace rime

#endif  // RIME_MATCHER_H_
