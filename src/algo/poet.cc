// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// simplistic sentence-making
// 
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/algo/poet.h>

namespace rime {

shared_ptr<DictEntry> Poet::MakeSentence(const WordGraph& graph,
                                         size_t total_length) {
  const int kMaxHomophonesInMind = 1;  // 20 if we had bigram model...
  const double kEpsilon = 1e-30;
  const double kPenalty = 1e-8;
  std::map<int, shared_ptr<DictEntry> > sentence;
  sentence[0].reset(new DictEntry);
  sentence[0]->weight = 1.0;
  // dynamic programming
  BOOST_FOREACH(const WordGraph::value_type& w, graph) {
    size_t start_pos = w.first;
    EZDBGONLYLOGGERVAR(start_pos);
    if (sentence.find(start_pos) == sentence.end())
      continue;
    BOOST_FOREACH(const UserDictEntryCollector::value_type& x, w.second) {
      size_t end_pos = x.first;
      if (start_pos == 0 && end_pos == total_length)
        continue;  // exclude single words from the result
      EZDBGONLYLOGGERVAR(end_pos);
      const DictEntryList &entries(x.second);
      for (size_t i = 0; i < kMaxHomophonesInMind && i < entries.size(); ++i) {
        const shared_ptr<DictEntry> &e(entries[i]);
        shared_ptr<DictEntry> new_sentence(
            new DictEntry(*sentence[start_pos]));
        new_sentence->code.insert(new_sentence->code.end(),
                                  e->code.begin(), e->code.end());
        new_sentence->text.append(e->text);
        new_sentence->weight *= (std::max)(e->weight, kEpsilon) * kPenalty;
        if (sentence.find(end_pos) == sentence.end() ||
            sentence[end_pos]->weight < new_sentence->weight) {
          EZDBGONLYLOGGERPRINT("updated sentence[%d] with '%s', %g",
                               end_pos,
                               new_sentence->text.c_str(),
                               new_sentence->weight);
          sentence[end_pos] = new_sentence;
        }
      }
    }
  }
  if (sentence.find(total_length) == sentence.end())
    return shared_ptr<DictEntry>();
  else
    return sentence[total_length];
}

}  // namespace rime
