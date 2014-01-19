//
// Copyleft RIME Developers
// License: GPLv3
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_POET_H_
#define RIME_POET_H_

#include <rime/dict/user_dictionary.h>
#include <rime/gear/translator_commons.h>

namespace rime {

typedef std::map<int, UserDictEntryCollector> WordGraph;

class Language;

class Poet {
 public:
  Poet(Language* language) : language_(language) {}
  
  shared_ptr<Sentence> MakeSentence(const WordGraph& graph,
                                    size_t total_length);
 protected:
  Language* language_;
};  

}  // namespace rime

#endif  // RIME_POET_H_
