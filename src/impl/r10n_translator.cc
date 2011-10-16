// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// Romanization translator
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
// 2011-10-06 GONG Chen <chen.sst@gmail.com>  implemented simplistic sentence-making
//
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/r10n_translator.h>
#include <rime/impl/syllablizer.h>

namespace rime {

class R10nTranslation : public Translation {
 public:
  R10nTranslation(shared_ptr<DictEntry>& sentence,
                  shared_ptr<DictEntryCollector>& collector,
                  int start,
                  int total_length)
      : sentence_(sentence), collector_(collector),
        start_(start), total_length_(total_length) {
    set_exhausted(!sentence_ && (!collector_ || collector_->empty()));
  }

  virtual bool Next() {
    if (exhausted())
      return false;
    if (sentence_) {
      sentence_.reset();
      set_exhausted(!collector_ || collector_->empty());
    }
    else {
      DictEntryIterator &iter(collector_->rbegin()->second);
      if (!iter.Next()) {
        int consumed_input_length = collector_->rbegin()->first;
        EZLOGGERVAR(consumed_input_length);
        collector_->erase(consumed_input_length);
        EZLOGGERVAR(collector_->size());
        set_exhausted(!collector_ || collector_->empty());
      }
    }
    return exhausted();
  }

  virtual shared_ptr<Candidate> Peek() {
    if (exhausted())
      return shared_ptr<Candidate>();
    if (sentence_) {
      EZLOGGERVAR(sentence_->text);
      shared_ptr<Candidate> cand(new Candidate(
          "zh",
          sentence_->text,
          "",
          start_,
          start_ + total_length_,
          0));
      return cand;
    }
    else {
      int consumed_input_length = collector_->rbegin()->first;
      EZLOGGERVAR(consumed_input_length);
      DictEntryIterator &iter(collector_->rbegin()->second);
      const shared_ptr<DictEntry> &e(iter.Peek());
      EZLOGGERVAR(e->text);
      shared_ptr<Candidate> cand(new Candidate(
          "zh",
          e->text,
          e->prompt,
          start_,
          start_ + consumed_input_length,
          0));
      return cand;
    }
  }

 private:
  shared_ptr<DictEntry> sentence_;
  shared_ptr<DictEntryCollector> collector_;
  int start_;
  int total_length_;
};

R10nTranslator::R10nTranslator(Engine *engine)
    : Translator(engine) {
  if (!engine)
    return;
  Config *config = engine->schema()->config();
  std::string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    EZLOGGERPRINT("Error: dictionary not specified in schema '%s'.",
                  engine->schema()->schema_id().c_str());
    return;
  }
  dict_.reset(new Dictionary(dict_name));
  dict_->Load();
}

R10nTranslator::~R10nTranslator() {
}

Translation* R10nTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("abc"))
    return NULL;
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);

  Syllablizer syllablizer;
  SyllableGraph syllable_graph;
  int consumed = syllablizer.BuildSyllableGraph(input,
                                                *dict_->prism(),
                                                &syllable_graph);

  shared_ptr<DictEntryCollector> collector(dict_->Lookup(syllable_graph, 0));
  if (!collector) return NULL;

  shared_ptr<DictEntry> sentence;
  // strategy 1: make sentences when there is no exact-matching phrase candidate
  // strategy 2: make sentences in addition to existing phrase candidates
  if (!collector->empty() &&
      collector->rbegin()->first < consumed &&
      syllable_graph.edges.size() > 1) {
    sentence = SimplisticSentenceMaking(syllable_graph);
  }
  return new R10nTranslation(sentence, collector, segment.start, consumed);
}

shared_ptr<DictEntry> R10nTranslator::SimplisticSentenceMaking(const SyllableGraph& syllable_graph) {
  typedef std::map<int, shared_ptr<DictEntryCollector> > WordGraph;
  const int kMaxNumOfSentenceMakingHomophones = 1;  // 20; if we have bigram model...
  const double kEpsilon = 1e-30;
  const double kPenalty = 1e-8;
  int total_length = syllable_graph.interpreted_length;
  WordGraph graph;
  BOOST_FOREACH(const EdgeMap::value_type &v, syllable_graph.edges) {
    graph[v.first] = dict_->Lookup(syllable_graph, v.first);
  }
  std::map<int, shared_ptr<DictEntry> > sentence;
  sentence[0].reset(new DictEntry);
  sentence[0]->weight = 1.0;
  // dynamic programming
  BOOST_FOREACH(WordGraph::value_type &w, graph) {
    int start_pos = w.first;
    EZLOGGERVAR(start_pos);
    if (sentence.find(start_pos) == sentence.end())
      continue;
    BOOST_FOREACH(DictEntryCollector::value_type &x, *w.second) {
      int end_pos = x.first;
      if (start_pos == 0 && end_pos == total_length)  // exclude single words from the result
        continue;
      EZLOGGERVAR(end_pos);
      DictEntryIterator &it(x.second);
      for (int count = 0; count < kMaxNumOfSentenceMakingHomophones && !it.exhausted(); ++count) {
        shared_ptr<DictEntry> e(it.Peek());
        shared_ptr<DictEntry> new_sentence(new DictEntry(*sentence[start_pos]));
        new_sentence->code.insert(new_sentence->code.end(), e->code.begin(), e->code.end());
        new_sentence->text.append(e->text);
        new_sentence->weight *= (std::max)(e->weight, kEpsilon) * kPenalty;
        if (sentence.find(end_pos) == sentence.end() ||
            sentence[end_pos]->weight < new_sentence->weight) {
          EZLOGGERPRINT("updated sentence[%d] with '%s', %g",
                        end_pos, new_sentence->text.c_str(), new_sentence->weight);
          sentence[end_pos] = new_sentence;
        }
        it.Next();
      }
    }
  }
  if (sentence.find(total_length) == sentence.end())
    return shared_ptr<DictEntry>();
  else
    return sentence[total_length];
}

}  // namespace rime
