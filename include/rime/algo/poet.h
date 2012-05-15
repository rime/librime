// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_POET_H_
#define RIME_POET_H_

#include <map>
#include <vector>
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/candidate.h>
#include <rime/dict/vocabulary.h>
#include <rime/dict/user_dictionary.h>

namespace rime {

typedef std::map<int, UserDictEntryCollector> WordGraph;

template<class Sentence>
class Poet {
 public:
  shared_ptr<Sentence> MakeSentence(const WordGraph& graph,
                                    size_t total_length) {
    const int kMaxHomophonesInMind = 1;
    std::map<int, shared_ptr<Sentence> > sentences;
    sentences[0] = make_shared<Sentence>();
    // dynamic programming
    BOOST_FOREACH(const WordGraph::value_type& w, graph) {
      size_t start_pos = w.first;
      EZDBGONLYLOGGERVAR(start_pos);
      if (sentences.find(start_pos) == sentences.end())
        continue;
      BOOST_FOREACH(const UserDictEntryCollector::value_type& x, w.second) {
        size_t end_pos = x.first;
        if (start_pos == 0 && end_pos == total_length)
          continue;  // exclude single words from the result
        EZDBGONLYLOGGERVAR(end_pos);
        const DictEntryList &entries(x.second);
        for (size_t i = 0; i < kMaxHomophonesInMind && i < entries.size(); ++i) {
          const shared_ptr<DictEntry> &e(entries[i]);
          shared_ptr<Sentence> new_sentence = make_shared<Sentence>(*sentences[start_pos]);
          new_sentence->Extend(*e, end_pos);
          if (sentences.find(end_pos) == sentences.end() ||
              sentences[end_pos]->weight() < new_sentence->weight()) {
            EZDBGONLYLOGGERPRINT("updated sentences[%d] with '%s', %g",
                                 end_pos,
                                 new_sentence->text().c_str(),
                                 new_sentence->weight());
            sentences[end_pos] = new_sentence;
          }
        }
      }
    }
    if (sentences.find(total_length) == sentences.end())
      return shared_ptr<Sentence>();
    else
      return sentences[total_length];
  }
};  

}  // namespace rime

#endif  // RIME_POET_H_
