// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
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
  explicit Matcher(Engine *engine);

  virtual bool Proceed(Segmentation *segmentation);

 protected:
  RecognizerPatterns patterns_;
};

}  // namespace rime

#endif  // RIME_MATCHER_H_
