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
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <rime/composition.h>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/translation.h>
#include <rime/impl/dictionary.h>
#include <rime/impl/r10n_translator.h>
#include <rime/impl/syllablizer.h>
#include <rime/impl/user_dictionary.h>

namespace rime {

class R10nTranslation;

class R10nCandidate : public Candidate {
 public:
  R10nCandidate(int start, int end, const shared_ptr<DictEntry> &entry, const R10nTranslation *translation)
      : Candidate("zh", start, end), entry_(entry), translation_(translation) {}
  virtual const char* text() const {
    return entry_->text.c_str();
  }
  virtual const char* comment() const {
    return entry_->comment.c_str();
  }
  virtual const char* preedit() const {
    // TODO:
    return NULL;
  }
  const Code& code() const {
    return entry_->code;
  }
 protected:
  const shared_ptr<DictEntry> entry_;
  const R10nTranslation *translation_;
};

class R10nTranslation : public Translation {
 public:
  R10nTranslation(const std::string &input, int start, const std::string &delimiters)
      : input_(input), start_(start), delimiters_(delimiters) {
    set_exhausted(true);
  }

  bool Evaluate(Dictionary *dict, UserDictionary *user_dict);
  
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();

 protected:
  void CheckEmpty();
  shared_ptr<DictEntry> SimplisticSentenceMaking(Dictionary *dict,
                                                 UserDictionary *user_dict);

  const std::string input_;
  int start_;
  const std::string delimiters_;
  
  SyllableGraph syllable_graph_;
  shared_ptr<DictEntryCollector> phrase_;
  shared_ptr<UserDictEntryCollector> user_phrase_;
  
  DictEntryCollector::reverse_iterator phrase_iter_;
  UserDictEntryCollector::reverse_iterator user_phrase_iter_;
  std::set<std::string> candidate_set_;
};

class SelectSequence : public std::vector<shared_ptr<R10nCandidate> > {
};

// R10nTranslator implementation

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
    if (user_dict_) {
      user_dict_->Load();
      if (dict_)
        user_dict_->Attach(dict_->table(), dict_->prism());
    }
  }

  connection_ = engine->context()->commit_notifier().connect(
      boost::bind(&R10nTranslator::OnCommit, this, _1));
}

R10nTranslator::~R10nTranslator() {
  connection_.disconnect();
}

Translation* R10nTranslator::Query(const std::string &input, const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("abc"))
    return NULL;
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);

  R10nTranslation* result(new R10nTranslation(input, segment.start, delimiters_));
  if (result && result->Evaluate(dict_.get(), user_dict_.get()))
    return result;
  else
    return NULL;
}

void R10nTranslator::OnCommit(Context *ctx) {
  DictEntry commit_entry;
  BOOST_FOREACH(Composition::value_type &seg, *ctx->composition()) {
    const shared_ptr<R10nCandidate> cand = As<R10nCandidate>(seg.GetSelectedCandidate());
    if (cand) {
      commit_entry.text += cand->text();
      commit_entry.code.insert(commit_entry.code.end(),
                               cand->code().begin(), cand->code().end());
    }
    if ((!cand || seg.status >= Segment::kConfirmed) && !commit_entry.text.empty()) {
      EZLOGGERVAR(commit_entry.text);
      user_dict_->UpdateEntry(commit_entry, 1);
      commit_entry.text.clear();
      commit_entry.code.clear();
    }
  }
}

// R10nTranslation implementation

bool R10nTranslation::Evaluate(Dictionary *dict, UserDictionary *user_dict) {
  Syllablizer syllablizer(delimiters_, true);
  int consumed = syllablizer.BuildSyllableGraph(input_,
                                                *dict->prism(),
                                                &syllable_graph_);

  phrase_ = dict->Lookup(syllable_graph_, 0);
  user_phrase_ = user_dict->Lookup(syllable_graph_, 0);
  if (!phrase_ && !user_phrase_)
    return false;
  // make sentences when there is no exact-matching phrase candidate
  int translated_len = 0;
  if (phrase_ && !phrase_->empty())
    translated_len = (std::max)(translated_len, phrase_->rbegin()->first);
  if (user_phrase_ && !user_phrase_->empty())
    translated_len = (std::max)(translated_len, user_phrase_->rbegin()->first);
  if (translated_len < consumed &&
      syllable_graph_.edges.size() > 1) {  // at least 2 syllables required
    shared_ptr<DictEntry> sentence = SimplisticSentenceMaking(dict, user_dict);
    if (sentence) {
      if (!user_phrase_) user_phrase_.reset(new UserDictEntryCollector);
      (*user_phrase_)[consumed].push_back(sentence);
    }
  }

  if (phrase_) phrase_iter_ = phrase_->rbegin();
  if (user_phrase_) user_phrase_iter_ = user_phrase_->rbegin();
  CheckEmpty();
  return !exhausted();
}

bool R10nTranslation::Next() {
  if (exhausted())
    return false;
  do {
    int user_phrase_code_length = 0;
    if (user_phrase_ && user_phrase_iter_ != user_phrase_->rend()) {
      user_phrase_code_length = user_phrase_iter_->first;
    }
    int phrase_code_length = 0;
    if (phrase_ && phrase_iter_ != phrase_->rend()) {
      phrase_code_length = phrase_iter_->first;
    }
    if (user_phrase_code_length > 0 &&
        user_phrase_code_length > phrase_code_length) {
      DictEntryList &entries(user_phrase_iter_->second);
      candidate_set_.insert(entries.back()->text);
      entries.pop_back();
      if (entries.empty()) {
        ++user_phrase_iter_;
      }
    }
    else if (phrase_code_length > 0) {
      DictEntryIterator &iter(phrase_iter_->second);
      candidate_set_.insert(iter.Peek()->text);
      if (!iter.Next()) {
        ++phrase_iter_;
      }
    }
    CheckEmpty();
  }
  while (!exhausted() && /* skip duplicate candidates */
         candidate_set_.find(Peek()->text()) != candidate_set_.end());
  return exhausted();
}

shared_ptr<Candidate> R10nTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  int user_phrase_code_length = 0;
  if (user_phrase_ && user_phrase_iter_ != user_phrase_->rend()) {
    user_phrase_code_length = user_phrase_iter_->first;
  }
  EZLOGGERVAR(user_phrase_code_length);
  int phrase_code_length = 0;
  if (phrase_ && phrase_iter_ != phrase_->rend()) {
    phrase_code_length = phrase_iter_->first;
  }
  EZLOGGERVAR(phrase_code_length);
  if (user_phrase_code_length > 0 &&
      user_phrase_code_length > phrase_code_length) {
    DictEntryList &entries(user_phrase_iter_->second);
    const shared_ptr<DictEntry> &e(entries.back());
    EZLOGGERVAR(e->text);
    shared_ptr<Candidate> cand(new R10nCandidate(
        start_,
        start_ + user_phrase_code_length,
        e,
        this));
    return cand;
  }
  if (phrase_code_length > 0) {
    DictEntryIterator &iter(phrase_iter_->second);
    const shared_ptr<DictEntry> &e(iter.Peek());
    EZLOGGERVAR(e->text);
    shared_ptr<Candidate> cand(new R10nCandidate(
        start_,
        start_ + phrase_code_length,
        e,
        this));
    return cand;
  }
  return shared_ptr<Candidate>();
}

void R10nTranslation::CheckEmpty() {
  set_exhausted((!phrase_ || phrase_iter_ == phrase_->rend()) &&
                (!user_phrase_ || user_phrase_iter_ == user_phrase_->rend()));
}

shared_ptr<DictEntry> R10nTranslation::SimplisticSentenceMaking(Dictionary *dict,
                                                                UserDictionary *user_dict) {
  typedef std::map<int, UserDictEntryCollector> WordGraph;
  const int kMaxNumOfSentenceMakingHomophones = 1;  // 20; if we have bigram model...
  const double kEpsilon = 1e-30;
  const double kPenalty = 1e-8;
  int total_length = syllable_graph_.interpreted_length;
  WordGraph graph;
  BOOST_FOREACH(const EdgeMap::value_type &s, syllable_graph_.edges) {
    shared_ptr<UserDictEntryCollector> user_phrase = user_dict->Lookup(syllable_graph_, s.first);
    UserDictEntryCollector &u(graph[s.first]);
    if (user_phrase)
      u.swap(*user_phrase);
    shared_ptr<DictEntryCollector> phrase = dict->Lookup(syllable_graph_, s.first);
    if (phrase) {
      // merge lookup results
      BOOST_FOREACH(DictEntryCollector::value_type &t, *phrase) {
        DictEntryList &entries(u[t.first]);
        if (entries.empty()) {
          shared_ptr<DictEntry> e(t.second.Peek());
          entries.push_back(e);
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
        const shared_ptr<DictEntry> &e(entries.back());
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
