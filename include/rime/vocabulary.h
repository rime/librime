// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-11 Wensong He <snowhws@gmail.com>
//

#ifndef VOCABULARY_H_
#define VOCABULARY_H_

#include <rime/schema.h>

namespace rime {

class Vocabulary {
 public:
  Vocabulary(){};
  Vocabulary(const Schema *schema) : schema_(schema) {}
  virtual ~Vocabulary() {}

  //virtual bool FindWords();
  
 protected:
  const Schema *schema_;
};

}  // namespace rime

#endif  // VOCABULARY_H_




