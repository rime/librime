//
// Copyright RIME Developers
// Distributed under the BSD License
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

using WordGraph = map<int, UserDictEntryCollector>;

class Language;

class Poet {
 public:
  Poet(Language* language) : language_(language) {}

  an<Sentence> MakeSentence(const WordGraph& graph,
                                    size_t total_length);
 protected:
  Language* language_;
};

}  // namespace rime

#endif  // RIME_POET_H_
