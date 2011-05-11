// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-11 Wensong He <snowhws@gmail.com>
//

#ifndef TRIVIAL_VOCABULARY_H_
#define TRIVIAL_VOCABULARY_H_

#include <rime/vocabulary.h>

namespace rime {

class TrivialVocabulary : public Vocabulary {
 public:
  TrivialSegmentor(){};
  TrivialSegmentor(const Schema *schema) : schema_(schema) {}
  
  //virtual bool FindWord();

 protected:
};

}  // namespace rime

#endif  // TRIVIAL_VOCABULARY_H_




