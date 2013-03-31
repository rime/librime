//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// Romanization translator
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <boost/algorithm/string/join.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/foreach.hpp>
#include <rime/composition.h>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/dict/dictionary.h>
#include <rime/algo/syllabifier.h>
#include <rime/gear/poet.h>
#include <rime/gear/r10n_translator.h>
#include <rime/gear/translator_commons.h>


static const char *quote_left = "\xef\xbc\x88";
static const char *quote_right = "\xef\xbc\x89";

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

class R10nTranslation : public Translation, public Syllabification,
                        public boost::enable_shared_from_this<R10nTranslation>
{
 public:
  R10nTranslation(R10nTranslator *translator,
                  const std::string &input, size_t start)
      : translator_(translator),
        input_(input), start_(start),
        user_phrase_index_(0) {
    set_exhausted(true);
  }
  bool Evaluate(Dictionary *dict, UserDictionary *user_dict);
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();
  virtual size_t PreviousStop(size_t caret_pos) const;
  virtual size_t NextStop(size_t caret_pos) const;

 protected:
  bool CheckEmpty();
  template <class CandidateT>
  const std::string GetPreeditString(const CandidateT &cand) const;
  template <class CandidateT>
  const std::string GetOriginalSpelling(const CandidateT &cand) const;
  const shared_ptr<Sentence> MakeSentence(Dictionary *dict,
                                          UserDictionary *user_dict);

  R10nTranslator *translator_;
  const std::string input_;
  size_t start_;

  SyllableGraph syllable_graph_;
  shared_ptr<DictEntryCollector> phrase_;
  shared_ptr<UserDictEntryCollector> user_phrase_;
  shared_ptr<Sentence> sentence_;

  DictEntryCollector::reverse_iterator phrase_iter_;
  UserDictEntryCollector::reverse_iterator user_phrase_iter_;
  size_t user_phrase_index_;
};

// R10nTranslator implementation

R10nTranslator::R10nTranslator(const TranslatorTicket& ticket)
    : Translator(ticket),
      Memory(engine_, name_space_),
      TranslatorOptions(engine_, name_space_),
      spelling_hints_(0) {
  if (!engine_) return;
  Config *config = engine_->schema()->config();
  if (config) {
    config->GetInt(name_space_ + "/spelling_hints", &spelling_hints_);
  }
}

shared_ptr<Translation> R10nTranslator::Query(const std::string &input,
                                              const Segment &segment,
                                              std::string* prompt) {
  if (!dict_ || !dict_->loaded())
    return shared_ptr<Translation>();
  if (!segment.HasTag("abc"))
    return shared_ptr<Translation>();
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  bool enable_user_dict = user_dict_ && user_dict_->loaded() &&
      !IsUserDictDisabledFor(input);

  // the translator should survive translations it creates
  shared_ptr<R10nTranslation> result =
      boost::make_shared<R10nTranslation>(this, input, segment.start);
  if (!result ||
      !result->Evaluate(dict_.get(),
                        enable_user_dict ? user_dict_.get() : NULL)) {
    return shared_ptr<Translation>();
  }
  return make_shared<UniqueFilter>(result);
}

const std::string R10nTranslator::FormatPreedit(const std::string& preedit) {
  std::string result(preedit);
  preedit_formatter_.Apply(&result);
  return result;
}

const std::string R10nTranslator::Spell(const Code &code) {
  std::string result;
  dictionary::RawCode syllables;
  if (!dict_ || !dict_->Decode(code, &syllables) || syllables.empty())
    return result;
  result =  boost::algorithm::join(syllables,
                                   std::string(1, delimiters_.at(0)));
  comment_formatter_.Apply(&result);
  return result;
}

bool R10nTranslator::Memorize(const CommitEntry& commit_entry) {
  bool update_elements = false;
  // avoid updating single character entries within a phrase which is
  // composed with single characters only
  if (commit_entry.elements.size() > 1) {
    BOOST_FOREACH(const DictEntry* e, commit_entry.elements) {
      if (e->code.size() > 1) {
        update_elements = true;
        break;
      }
    }
  }
  if (update_elements) {
    BOOST_FOREACH(const DictEntry* e, commit_entry.elements) {
      user_dict_->UpdateEntry(*e, 0);
    }
  }
  user_dict_->UpdateEntry(commit_entry, 1);
  return true;
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
  return !CheckEmpty();
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

template <class CandidateT>
const std::string R10nTranslation::GetOriginalSpelling(
    const CandidateT& cand) const {
  if (translator_ &&
      static_cast<int>(cand.code().size()) <= translator_->spelling_hints()) {
    return translator_->Spell(cand.code());
  }
  return std::string();
}

bool R10nTranslation::Next() {
  if (exhausted())
    return false;
  if (sentence_) {
    sentence_.reset();
    return !CheckEmpty();
  }
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
    if (++user_phrase_index_ >= entries.size()) {
      ++user_phrase_iter_;
      user_phrase_index_ = 0;
    }
  }
  else if (phrase_code_length > 0) {
    DictEntryIterator &iter(phrase_iter_->second);
    if (!iter.Next()) {
      ++phrase_iter_;
    }
  }
  return !CheckEmpty();
}

shared_ptr<Candidate> R10nTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  if (sentence_) {
    if (sentence_->preedit().empty()) {
      sentence_->set_preedit(GetPreeditString(*sentence_));
    }
    if (sentence_->comment().empty()) {
      const std::string spelling(GetOriginalSpelling(*sentence_));
      if (!spelling.empty() &&
          spelling != sentence_->preedit()) {
        sentence_->set_comment(quote_left + spelling + quote_right);
      }
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
  shared_ptr<Phrase> cand;
  if (user_phrase_code_length > 0 &&
      user_phrase_code_length >= phrase_code_length) {
    DictEntryList &entries(user_phrase_iter_->second);
    const shared_ptr<DictEntry> &e(entries[user_phrase_index_]);
    DLOG(INFO) << "user phrase '" << e->text
               << "', code length: " << user_phrase_code_length;
    cand = make_shared<Phrase>(translator_->language(),
                               "phrase",
                               start_,
                               start_ + user_phrase_code_length,
                               e);
  }
  else if (phrase_code_length > 0) {
    DictEntryIterator &iter(phrase_iter_->second);
    const shared_ptr<DictEntry> &e(iter.Peek());
    DLOG(INFO) << "phrase '" << e->text
               << "', code length: " << user_phrase_code_length;
    cand = make_shared<Phrase>(translator_->language(),
                               "phrase",
                               start_,
                               start_ + phrase_code_length,
                               e);
  }
  if (cand->preedit().empty()) {
    cand->set_preedit(GetPreeditString(*cand));
  }
  if (cand->comment().empty()) {
    const std::string spelling(GetOriginalSpelling(*cand));
    if (!spelling.empty() &&
        spelling != cand->preedit()) {
      cand->set_comment(quote_left + spelling + quote_right);
    }
  }
  cand->set_syllabification(shared_from_this());
  return cand;
}

bool R10nTranslation::CheckEmpty() {
  set_exhausted((!phrase_ || phrase_iter_ == phrase_->rend()) &&
                (!user_phrase_ || user_phrase_iter_ == user_phrase_->rend()));
  return exhausted();
}

const shared_ptr<Sentence> R10nTranslation::MakeSentence(
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
  Poet poet(translator_->language());
  shared_ptr<Sentence> sentence =
      poet.MakeSentence(graph, syllable_graph_.interpreted_length);
  if (sentence) {
    sentence->Offset(start_);
    sentence->set_syllabification(shared_from_this());
  }
  return sentence;
}

size_t R10nTranslation::PreviousStop(size_t caret_pos) const {
  size_t offset = caret_pos - start_;
  BOOST_REVERSE_FOREACH(const VertexMap::value_type& x,
                        syllable_graph_.vertices) {
    if (x.first < offset)
      return x.first + start_;
  }
  return caret_pos;
}

size_t R10nTranslation::NextStop(size_t caret_pos) const {
  size_t offset = caret_pos - start_;
  BOOST_FOREACH(const VertexMap::value_type& x, syllable_graph_.vertices) {
    if (x.first > offset)
      return x.first + start_;
  }
  return caret_pos;
}

}  // namespace rime
