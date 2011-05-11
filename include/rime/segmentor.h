// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-11 Wensong He <snowhws@gmail.com>
//

#ifndef SEGMENTOR_H_
#define SEGMENTOR_H_

#include <rime/schema.h>

namespace rime {

class Segmentor {
 public:
  Segmentor(){};
  Segmentor(const Schema *schema) : schema_(schema) {}
  virtual ~Segmentor() {}
  //@input the string need to segment
  //@result result pointer of segmenting 
  virtual bool segment(std::string input,void *result);

 protected:
  const Schema *schema_;
};

}  // namespace rime

#endif  // SEGMENTOR_H_



