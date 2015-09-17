//
// Copyright RIME Developers
// Distributed under the BSD License
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/candidate.h>
#include <rime/dict/vocabulary.h>
#include <rime/gear/poet.h>

namespace rime {

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                        size_t total_length) {
  const int kMaxHomophonesInMind = 1;
  map<int, an<Sentence>> sentences;
  sentences[0] = New<Sentence>(language_);
  // dynamic programming
  for (const auto& w : graph) {
    size_t start_pos = w.first;
    DLOG(INFO) << "start pos: " << start_pos;
    if (sentences.find(start_pos) == sentences.end())
      continue;
    for (const auto& x : w.second) {
      size_t end_pos = x.first;
      if (start_pos == 0 && end_pos == total_length)
        continue;  // exclude single words from the result
      DLOG(INFO) << "end pos: " << end_pos;
      const DictEntryList& entries(x.second);
      for (size_t i = 0; i < kMaxHomophonesInMind && i < entries.size(); ++i) {
        const auto& entry(entries[i]);
        auto new_sentence = New<Sentence>(*sentences[start_pos]);
        new_sentence->Extend(*entry, end_pos);
        if (sentences.find(end_pos) == sentences.end() ||
            sentences[end_pos]->weight() < new_sentence->weight()) {
          DLOG(INFO) << "updated sentences " << end_pos << ") with '"
                     << new_sentence->text() << "', " << new_sentence->weight();
          sentences[end_pos] = new_sentence;
        }
      }
    }
  }
  if (sentences.find(total_length) == sentences.end())
    return nullptr;
  else
    return sentences[total_length];
}

}  // namespace rime
