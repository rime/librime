// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-11 Wensong He <snowhws@gmail.com>
//

#ifndef TRIVIAL_SEGMENTOR_H_
#define TRIVIAL_SEGMENTOR_H_

#include <rime/segment.h>

namespace rime {
class TrivialSegmentorResult {
  //****
}

class TrivialSegmentor : public Segmentor {
 public:
  TrivialSegmentor(){};
  TrivialSegmentor(const Schema *schema) : schema_(schema) {}
  //@input the string need to segment
  //@result result pointer of segmenting 
  virtual bool segment(std::string input,void *result);

 protected:
};

}  // namespace rime

#endif  // SEGMENTOR_H_




