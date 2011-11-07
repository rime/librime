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
#include <algorithm>
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
#include <rime/impl/user_dictionary.h>

namespace rime {

class R10nTranslation : public Translation {
 public:
  R10nTranslation(shared_ptr<DictEntryCollector>& phrase,
                  shared_ptr<UserDictEntryCollector>& user_phrase,
                  int start,
                  int total_length)
      : phrase_(phrase), user_phrase_(user_phrase),
        start_(start), total_length_(total_length) {
    if (phrase_) phrase_iter_ = phrase_->rbegin();
    if (user_phrase_) user_phrase_iter_ = user_phrase_->rbegin();
    CheckEmpty();
  }

  virtual bool Next() {
    if (exhausted())
      return false;
    if (user_phrase_ && user_phrase_iter_ != user_phrase_->rend()) {
      DictEntryList &entries(user_phrase_iter_->second);
      entries.pop_back();
      if (entries.empty()) {
        ++user_phrase_iter_;
        CheckEmpty();
      }
      return exhausted();
    }
    if (phrase_ && phrase_iter_ != phrase_->rend()) {
      DictEntryIterator &iter(phrase_iter_->second);
      if (!iter.Next()) {
        ++phrase_iter_;
        CheckEmpty();
      }
    }
    return exhausted();
  }

  virtual shared_ptr<Candidate> Peek() {
    if (exhausted())
      return shared_ptr<Candidate>();
    if (user_phrase_ && user_phrase_iter_ != user_phrase_->rend()) {
      int consumed_input_length = user_phrase_iter_->first;
      EZLOGGERVAR(consumed_input_length);
      DictEntryList &entries(user_phrase_iter_->second);
      const DictEntry &e(entries.back());
      EZLOGGERVAR(e.text);
      shared_ptr<Candidate> cand(new Candidate(
          "zh",
          e.text,
          e.prompt,
          start_,
          start_ + consumed_input_length,
          0));
      return cand;
    }
    if (phrase_ && phrase_iter_ != phrase_->rend()) {
      int consumed_input_length = phrase_iter_->first;
      EZLOGGERVAR(consumed_input_length);
      DictEntryIterator &iter(phrase_iter_->second);
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
    return shared_ptr<Candidate>();
  }

 private:
  void CheckEmpty() {
    set_exhausted((!phrase_ || phrase_iter_ == phrase_->rend()) &&
                  (!user_phrase_ || user_phrase_iter_ == user_phrase_->rend()));
  }
        
  shared_ptr<DictEntryCollector> phrase_;
  shared_ptr<UserDictEntryCollector> user_phrase_;
  DictEntryCollector::reverse_iterator phrase_iter_;
  UserDictEntryCollector::reverse_iterator user_phrase_iter_;
  int start_;
  int total_length_;
};

R10nTranslator::R10nTranslator(Engine *engine)
    : Translator(engine) {
  if (!engine) return;

  Config *config = engine->schema()->config();
  if (config)
    config->GetString("speller/delimiter", &delimiters_);
  
  Dictionary::Component *dictionary = Dictionary::Require("dictionary");
  if (dictionary) {
    dict_.reset(dictionary->Create(engine->schema()));
    if (dict_)
      dict_->Load();
  }

  UserDictionary::Component *user_dictionary = UserDictionary::Require("user_dictionary");
  if (user_dictionary) {
    user_dict_.reset(user_dictionary->Create(engine->schema()));
    if (user_dict_)
      user_dict_->Load();
  }
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

  Syllablizer syllablizer(delimiters_);
  SyllableGraph syllable_graph;
  int consumed = syllablizer.BuildSyllableGraph(input,
                                                *dict_->prism(),
                                                &syllable_graph);

  shared_ptr<DictEntryCollector> phrase(dict_->Lookup(syllable_graph, 0));
  shared_ptr<UserDictEntryCollector> user_phrase(user_dict_->Lookup(syllable_graph, 0));
  if (!phrase && !user_phrase)
    return NULL;
  // make sentences when there is no exact-matching phrase candidate
  int translated_len = 0;
  if (phrase && !phrase->empty())
    translated_len = (std::max)(translated_len, phrase->rbegin()->first);
  if (user_phrase && !user_phrase->empty())
    translated_len = (std::max)(translated_len, user_phrase->rbegin()->first);
  if (translated_len < consumed &&
      syllable_graph.edges.size() > 1) {  // at least 2 syllables required
    shared_ptr<DictEntry> sentence = SimplisticSentenceMaking(syllable_graph);
    if (sentence) {
      if (!user_phrase) user_phrase.reset(new UserDictEntryCollector);
      (*user_phrase)[consumed].push_back(*sentence);
    }
  }
  return new R10nTranslation(phrase, user_phrase, segment.start, consumed);
}

shared_ptr<DictEntry> R10nTranslator::SimplisticSentenceMaking(const SyllableGraph& syllable_graph) {
  typedef std::map<int, UserDictEntryCollector> WordGraph;
  const int kMaxNumOfSentenceMakingHomophones = 1;  // 20; if we have bigram model...
  const double kEpsilon = 1e-30;
  const double kPenalty = 1e-8;
  int total_length = syllable_graph.interpreted_length;
  WordGraph graph;
  BOOST_FOREACH(const EdgeMap::value_type &s, syllable_graph.edges) {
    shared_ptr<UserDictEntryCollector> user_phrase = user_dict_->Lookup(syllable_graph, s.first);
    UserDictEntryCollector &u(graph[s.first]);
    if (user_phrase)
      u.swap(*user_phrase);
    shared_ptr<DictEntryCollector> phrase = dict_->Lookup(syllable_graph, s.first);
    if (phrase) {
      // merge lookup results
      BOOST_FOREACH(DictEntryCollector::value_type &t, *phrase) {
        DictEntryList &entries(u[t.first]);
        if (entries.empty()) {
          shared_ptr<DictEntry> e(t.second.Peek());
          entries.push_back(*e);
        }
      }
    }
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
    BOOST_FOREACH(UserDictEntryCollector::value_type &x, w.second) {
      int end_pos = x.first;
      if (start_pos == 0 && end_pos == total_length)  // exclude single words from the result
        continue;
      EZLOGGERVAR(end_pos);
      DictEntryList &entries(x.second);
      for (int count = 0; count < kMaxNumOfSentenceMakingHomophones && !entries.empty(); ++count) {
        DictEntry &e(entries.back());
        shared_ptr<DictEntry> new_sentence(new DictEntry(*sentence[start_pos]));
        new_sentence->code.insert(new_sentence->code.end(), e.code.begin(), e.code.end());
        new_sentence->text.append(e.text);
        new_sentence->weight *= (std::max)(e.weight, kEpsilon) * kPenalty;
        if (sentence.find(end_pos) == sentence.end() ||
            sentence[end_pos]->weight < new_sentence->weight) {
          EZLOGGERPRINT("updated sentence[%d] with '%s', %g",
                        end_pos, new_sentence->text.c_str(), new_sentence->weight);
          sentence[end_pos] = new_sentence;
        }
        entries.pop_back();
      }
    }
  }
  if (sentence.find(total_length) == sentence.end())
    return shared_ptr<DictEntry>();
  else
    return sentence[total_length];
}

}  // namespace rime
