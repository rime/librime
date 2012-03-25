// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// Romanization translator
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
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
#include <rime/dict/dictionary.h>
#include <rime/algo/poet.h>
#include <rime/algo/syllabifier.h>
#include <rime/impl/r10n_translator.h>

namespace rime {

namespace {

struct DelimitSyllableState {
  const std::string *input;
  const std::string *delimiters;
  const SyllableGraph *graph;
  const Code *code;
  size_t end_pos;
  std::string output;
};

bool DelimitSyllablesDfs(DelimitSyllableState *state,
                         size_t current_pos, size_t depth) {
  if (depth == state->code->size()) {
    return current_pos == state->end_pos;
  }
  SyllableId syllable_id = state->code->at(depth);
  EdgeMap::const_iterator z = state->graph->edges.find(current_pos);
  if (z == state->graph->edges.end())
    return false;
  // favor longer spellings
  BOOST_REVERSE_FOREACH(const EndVertexMap::value_type &y, z->second) {
    size_t end_vertex_pos = y.first;
    if (end_vertex_pos > state->end_pos)
      continue;
    SpellingMap::const_iterator x = y.second.find(syllable_id);
    if (x != y.second.end()) {
      size_t len = state->output.length();
      if (depth > 0 && len > 0 &&
          state->delimiters->find(
              state->output[len - 1]) == std::string::npos) {
        state->output += state->delimiters->at(0);
      }
      state->output += state->input->substr(current_pos,
                                            end_vertex_pos - current_pos);
      if (DelimitSyllablesDfs(state, end_vertex_pos, depth + 1))
        return true;
      state->output.resize(len);
    }
  }
  return false;
}

}  // anonymous namespace

class R10nCandidate;
class R10nSentence;

class R10nTranslation : public Translation {
 public:
  R10nTranslation(const std::string &input, size_t start,
                  R10nTranslator *translator)
      : input_(input), start_(start),
        translator_(translator),
        user_phrase_index_(0) {
    set_exhausted(true);
  }
  bool Evaluate(Dictionary *dict, UserDictionary *user_dict);
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();

 protected:
  void CheckEmpty();
  template <class CandidateT>
  const std::string GetPreeditString(const CandidateT &cand) const;
  const shared_ptr<R10nSentence> MakeSentence(Dictionary *dict,
                                              UserDictionary *user_dict);

  const std::string input_;
  size_t start_;
  R10nTranslator *translator_;
  
  SyllableGraph syllable_graph_;
  shared_ptr<DictEntryCollector> phrase_;
  shared_ptr<UserDictEntryCollector> user_phrase_;
  shared_ptr<R10nSentence> sentence_;
  
  DictEntryCollector::reverse_iterator phrase_iter_;
  UserDictEntryCollector::reverse_iterator user_phrase_iter_;
  size_t user_phrase_index_;
  std::set<std::string> candidate_set_;
};

class R10nCandidate : public Candidate {
 public:
  R10nCandidate(size_t start, size_t end,
                const shared_ptr<DictEntry> &entry)
      : Candidate("zh", start, end),
        entry_(entry) {
  }
  const std::string& text() const { return entry_->text; }
  const std::string comment() const { return entry_->comment; }
  const std::string preedit() const { return entry_->preedit; }
  void set_preedit(const std::string &preedit) {
    entry_->preedit = preedit;
  }
  const Code& code() const { return entry_->code; }
  const DictEntry& entry() const { return *entry_; }
 protected:
  const shared_ptr<DictEntry> entry_;
};

class R10nSentence : public Candidate {
 public:
  R10nSentence() : Candidate("sentence", 0, 0) {
    entry_.weight = 1.0;
  }
  void Extend(const DictEntry& entry, size_t end_pos) {
    const double kEpsilon = 1e-200;
    const double kPenalty = 1e-8;
    entry_.code.insert(entry_.code.end(),
                       entry.code.begin(), entry.code.end());
    entry_.text.append(entry.text);
    entry_.weight *= (std::max)(entry.weight, kEpsilon) * kPenalty;
    components_.push_back(entry);
    set_end(end_pos);
    EZDBGONLYLOGGERPRINT("%d) %s : %g", end_pos,
                         entry_.text.c_str(), entry_.weight);
  }
  void Offset(size_t offset) {
    set_start(start() + offset);
    set_end(end() + offset);
  }
  const std::string& text() const { return entry_.text; }
  const std::string preedit() const { return entry_.preedit; }
  void set_preedit(const std::string &preedit) {
    entry_.preedit = preedit;
  }
  const Code& code() const { return entry_.code; }
  double weight() const { return entry_.weight; }
  const std::vector<DictEntry>& components() const { return components_; }
  
 protected:
  DictEntry entry_;
  std::vector<DictEntry> components_;
};

// R10nTranslator implementation

R10nTranslator::R10nTranslator(Engine *engine)
    : Translator(engine),
      enable_completion_(true) {
  if (!engine) return;

  Config *config = engine->schema()->config();
  if (config) {
    config->GetString("speller/delimiter", &delimiters_);
    config->GetBool("translator/enable_completion", &enable_completion_);
    formatter_.Load(config->GetList("translator/preedit_format"));
  }
  if (delimiters_.empty()) {
    delimiters_ = " ";
  }
  
  Dictionary::Component *dictionary = Dictionary::Require("dictionary");
  if (dictionary) {
    dict_.reset(dictionary->Create(engine->schema()));
    if (dict_)
      dict_->Load();
  }

  UserDictionary::Component *user_dictionary =
      UserDictionary::Require("user_dictionary");
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

Translation* R10nTranslator::Query(const std::string &input,
                                   const Segment &segment) {
  if (!dict_ || !dict_->loaded())
    return NULL;
  if (!segment.HasTag("abc"))
    return NULL;
  EZDBGONLYLOGGERPRINT("input = '%s', [%d, %d)",
                       input.c_str(), segment.start, segment.end);

  // the translator should survive translations it creates
  R10nTranslation* result(new R10nTranslation(input, segment.start, this));
  if (result && result->Evaluate(dict_.get(), user_dict_.get()))
    return result;
  else
    return NULL;
}

const std::string R10nTranslator::FormatPreedit(const std::string& preedit) {
  std::string result(preedit);
  formatter_.Apply(&result);
  return result;
}

void R10nTranslator::OnCommit(Context *ctx) {
  if (!user_dict_) return;
  DictEntry commit_entry;
  std::vector<const DictEntry*> elements;
  BOOST_FOREACH(Composition::value_type &seg, *ctx->composition()) {
    shared_ptr<Candidate> cand = seg.GetSelectedCandidate();
    bool unrecognized = false;
    shared_ptr<UniquifiedCandidate> uniquified = As<UniquifiedCandidate>(cand);
    if (uniquified) cand = uniquified->items().front();
    shared_ptr<ShadowCandidate> shadow = As<ShadowCandidate>(cand);
    if (shadow) cand = shadow->item();
    shared_ptr<R10nCandidate> r10n_cand = As<R10nCandidate>(cand);
    shared_ptr<R10nSentence> sentence = As<R10nSentence>(cand);
    if (r10n_cand) {
      commit_entry.text += r10n_cand->text();
      commit_entry.code.insert(commit_entry.code.end(),
                               r10n_cand->code().begin(),
                               r10n_cand->code().end());
      elements.push_back(&r10n_cand->entry());
    }
    else if (sentence) {
      commit_entry.text += sentence->text();
      commit_entry.code.insert(commit_entry.code.end(),
                               sentence->code().begin(),
                               sentence->code().end());
      BOOST_FOREACH(const DictEntry& e, sentence->components()) {
        elements.push_back(&e);
      }
    }
    else {
      unrecognized = true;
    }
    if ((unrecognized || seg.status >= Segment::kConfirmed) &&
        !commit_entry.text.empty()) {
      EZDBGONLYLOGGERVAR(commit_entry.text);
      bool update_elements = false;
      if (elements.size() > 1) {
        BOOST_FOREACH(const DictEntry* e, elements) {
          if (e->code.size() > 1) {
            update_elements = true;
            break;
          }
        }
      }
      if (update_elements) {
        BOOST_FOREACH(const DictEntry* e, elements) {
          user_dict_->UpdateEntry(*e, 0);
        }
      }
      elements.clear();
      user_dict_->UpdateEntry(commit_entry, 1);
      commit_entry.text.clear();
      commit_entry.code.clear();
    }
  }
}

// R10nTranslation implementation

bool R10nTranslation::Evaluate(Dictionary *dict, UserDictionary *user_dict) {
  Syllabifier syllabifier(translator_->delimiters(),
                          translator_->enable_completion());
  size_t consumed = syllabifier.BuildSyllableGraph(input_,
                                                   *dict->prism(),
                                                   &syllable_graph_);

  phrase_ = dict->Lookup(syllable_graph_, 0);
  if (user_dict) {
    user_phrase_ = user_dict->Lookup(syllable_graph_, 0);
  }
  if (!phrase_ && !user_phrase_)
    return false;
  // make sentences when there is no exact-matching phrase candidate
  size_t translated_len = 0;
  if (phrase_ && !phrase_->empty())
    translated_len = (std::max)(translated_len, phrase_->rbegin()->first);
  if (user_phrase_ && !user_phrase_->empty())
    translated_len = (std::max)(translated_len, user_phrase_->rbegin()->first);
  if (translated_len < consumed &&
      syllable_graph_.edges.size() > 1) {  // at least 2 syllables required
    sentence_ = MakeSentence(dict, user_dict);
  }

  if (phrase_) phrase_iter_ = phrase_->rbegin();
  if (user_phrase_) user_phrase_iter_ = user_phrase_->rbegin();
  CheckEmpty();
  return !exhausted();
}

template <class CandidateT>
const std::string R10nTranslation::GetPreeditString(
    const CandidateT &cand) const {
  DelimitSyllableState state;
  state.input = &input_;
  state.delimiters = &translator_->delimiters();
  state.graph = &syllable_graph_;
  state.code = &cand.code();
  state.end_pos = cand.end() - start_;
  bool success = DelimitSyllablesDfs(&state, cand.start() - start_, 0);
  if (success) {
    return translator_->FormatPreedit(state.output);
  }
  else {
    return std::string();
  }
}

bool R10nTranslation::Next() {
  if (exhausted())
    return false;
  if (sentence_) {
    candidate_set_.insert(sentence_->text());
    sentence_.reset();
    CheckEmpty();
    return exhausted();
  }
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
        user_phrase_code_length >= phrase_code_length) {
      DictEntryList &entries(user_phrase_iter_->second);
      candidate_set_.insert(entries[user_phrase_index_]->text);
      if (++user_phrase_index_ >= entries.size()) {
        ++user_phrase_iter_;
        user_phrase_index_ = 0;
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
  if (sentence_) {
    if (sentence_->preedit().empty()) {
      sentence_->set_preedit(GetPreeditString(*sentence_));
    }
    return sentence_;
  }
  size_t user_phrase_code_length = 0;
  if (user_phrase_ && user_phrase_iter_ != user_phrase_->rend()) {
    user_phrase_code_length = user_phrase_iter_->first;
  }
  size_t phrase_code_length = 0;
  if (phrase_ && phrase_iter_ != phrase_->rend()) {
    phrase_code_length = phrase_iter_->first;
  }
  shared_ptr<R10nCandidate> cand;
  if (user_phrase_code_length > 0 &&
      user_phrase_code_length >= phrase_code_length) {
    DictEntryList &entries(user_phrase_iter_->second);
    const shared_ptr<DictEntry> &e(entries[user_phrase_index_]);
    EZDBGONLYLOGGERVAR(user_phrase_code_length);
    EZDBGONLYLOGGERVAR(e->text);
    cand.reset(new R10nCandidate(start_,
                                 start_ + user_phrase_code_length,
                                 e));
  }
  else if (phrase_code_length > 0) {
    DictEntryIterator &iter(phrase_iter_->second);
    const shared_ptr<DictEntry> &e(iter.Peek());
    EZDBGONLYLOGGERVAR(phrase_code_length);
    EZDBGONLYLOGGERVAR(e->text);
    cand.reset(new R10nCandidate(start_,
                                 start_ + phrase_code_length,
                                 e));
  }
  if (cand && cand->preedit().empty()) {
    cand->set_preedit(GetPreeditString(*cand));
  }
  return cand;
}

void R10nTranslation::CheckEmpty() {
  set_exhausted((!phrase_ || phrase_iter_ == phrase_->rend()) &&
                (!user_phrase_ || user_phrase_iter_ == user_phrase_->rend()));
}

const shared_ptr<R10nSentence> R10nTranslation::MakeSentence(
    Dictionary *dict, UserDictionary *user_dict) {
  const int kMaxSyllablesForUserPhraseQuery = 5;
  const double kPenaltyForAmbiguousSyllable = 1e-10;
  WordGraph graph;
  BOOST_FOREACH(const EdgeMap::value_type &s, syllable_graph_.edges) {
    // discourage starting a word from an ambiguous joint
    // bad cases include pinyin syllabification "niju'ede"
    double credibility = 1.0;
    if (syllable_graph_.vertices[s.first] >= kAmbiguousSpelling)
      credibility = kPenaltyForAmbiguousSyllable;
    shared_ptr<UserDictEntryCollector> user_phrase;
    if (user_dict) {
      user_phrase = user_dict->Lookup(syllable_graph_, s.first,
                                      kMaxSyllablesForUserPhraseQuery,
                                      credibility);
    }
    UserDictEntryCollector &u(graph[s.first]);
    if (user_phrase)
      u.swap(*user_phrase);
    shared_ptr<DictEntryCollector> phrase =
        dict->Lookup(syllable_graph_, s.first, credibility);
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
  Poet<R10nSentence> poet;
  shared_ptr<R10nSentence> sentence =
      poet.MakeSentence(graph, syllable_graph_.interpreted_length);
  if (sentence) {
    sentence->Offset(start_);
  }
  return sentence;
}

}  // namespace rime
