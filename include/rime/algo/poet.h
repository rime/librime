// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_POET_H_
#define RIME_POET_H_

#include <map>
#include <rime/common.h>
#include <rime/dict/vocabulary.h>
#include <rime/dict/user_dictionary.h>

namespace rime {

typedef std::map<int, UserDictEntryCollector> WordGraph;

class Poet {
 public:
  shared_ptr<DictEntry> MakeSentence(const WordGraph& graph,
                                     size_t total_length);
};  

}  // namespace rime

#endif  // RIME_POET_H_
