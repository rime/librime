//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Script translator
//
// 2011-07-10 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <stack>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <rime/composition.h>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/corrector.h>
#include <rime/dict/dictionary.h>
#include <rime/gear/poet.h>
#include <rime/gear/script_translator.h>
#include <rime/gear/translator_commons.h>


//static const char* quote_left = "\xef\xbc\x88";
//static const char* quote_right = "\xef\xbc\x89";

namespace rime {

namespace {

struct SyllabifyTask {
  const Code& code;
  const SyllableGraph& graph;
  size_t target_pos;
  function<void (SyllabifyTask* task, size_t depth,
                      size_t current_pos, size_t next_pos)> push;
  function<void (SyllabifyTask* task, size_t depth)> pop;
};

static bool syllabify_dfs(SyllabifyTask* task,
                          size_t depth, size_t current_pos) {
  if (depth == task->code.size()) {
    return current_pos == task->target_pos;
  }
  SyllableId syllable_id = task->code.at(depth);
  auto z = task->graph.edges.find(current_pos);
  if (z == task->graph.edges.end())
    return false;
  // favor longer spellings
  for (const auto& y : boost::adaptors::reverse(z->second)) {
    size_t end_vertex_pos = y.first;
    if (end_vertex_pos > task->target_pos)
      continue;
    auto x = y.second.find(syllable_id);
    if (x != y.second.end()) {
      task->push(task, depth, current_pos, end_vertex_pos);
      if (syllabify_dfs(task, depth + 1, end_vertex_pos))
        return true;
      task->pop(task, depth);
    }
  }
  return false;
}

}  // anonymous namespace

class ScriptSyllabifier : public PhraseSyllabifier {
 public:
  ScriptSyllabifier(ScriptTranslator* translator,
                    Corrector* corrector,
                    const string& input,
                    size_t start)
      : translator_(translator), input_(input), start_(start),
        syllabifier_(translator->delimiters(),
                     translator->enable_completion(),
                     translator->strict_spelling()) {
    if (corrector) {
      syllabifier_.EnableCorrection(corrector);
    }
  }

  virtual Spans Syllabify(const Phrase* phrase);
  size_t BuildSyllableGraph(Prism& prism);
  string GetPreeditString(const Phrase& cand) const;
  string GetOriginalSpelling(const Phrase& cand) const;
  bool IsCandidateCorrection(const Phrase& cand) const;

  const SyllableGraph& syllable_graph() const { return syllable_graph_; }

 protected:
  ScriptTranslator* translator_;
  string input_;
  size_t start_;
  Syllabifier syllabifier_;
  SyllableGraph syllable_graph_;
};

class ScriptTranslation : public Translation {
 public:
  ScriptTranslation(ScriptTranslator* translator,
                    Corrector* corrector,
                    Poet* poet,
                    const string& input,
                    size_t start)
      : translator_(translator),
        poet_(poet),
        start_(start),
        syllabifier_(New<ScriptSyllabifier>(
            translator, corrector, input, start)),
        enable_correction_(corrector) {
    set_exhausted(true);
  }
  bool Evaluate(Dictionary* dict, UserDictionary* user_dict);
  virtual bool Next();
  virtual an<Candidate> Peek();

 protected:
  bool CheckEmpty();
  bool IsNormalSpelling() const;
  an<Sentence> MakeSentence(Dictionary* dict,
                                    UserDictionary* user_dict);
  void PrepareCandidate();

  ScriptTranslator* translator_;
  Poet* poet_;
  size_t start_;
  an<ScriptSyllabifier> syllabifier_;

  an<DictEntryCollector> phrase_;
  an<UserDictEntryCollector> user_phrase_;
  an<Sentence> sentence_;

  an<Phrase> candidate_ = nullptr;

  DictEntryCollector::reverse_iterator phrase_iter_;
  UserDictEntryCollector::reverse_iterator user_phrase_iter_;
  size_t user_phrase_index_ = 0;

  size_t max_corrections_ = 4;
  size_t correction_count_ = 0;

  bool enable_correction_;
};

// ScriptTranslator implementation

ScriptTranslator::ScriptTranslator(const Ticket& ticket)
    : Translator(ticket),
      Memory(ticket),
      TranslatorOptions(ticket) {
  if (!engine_)
    return;
  if (Config* config = engine_->schema()->config()) {
    config->GetInt(name_space_ + "/spelling_hints", &spelling_hints_);
    config->GetBool(name_space_ + "/always_show_comments",
                    &always_show_comments_);
    config->GetBool(name_space_ + "/enable_correction", &enable_correction_);
    config->GetInt(name_space_ + "/max_homophones", &max_homophones_);
    poet_.reset(new Poet(language(), config));
  }
  if (enable_correction_) {
    if (auto* corrector = Corrector::Require("corrector")) {
      corrector_.reset(corrector->Create(ticket));
    }
  }
}

an<Translation> ScriptTranslator::Query(const string& input,
                                        const Segment& segment) {
  if (!dict_ || !dict_->loaded())
    return nullptr;
  if (!segment.HasTag(tag_))
    return nullptr;
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  FinishSession();

  bool enable_user_dict = user_dict_ && user_dict_->loaded() &&
      !IsUserDictDisabledFor(input);

  // the translator should survive translations it creates
  auto result = New<ScriptTranslation>(this,
                                       corrector_.get(),
                                       poet_.get(),
                                       input,
                                       segment.start);
  if (!result ||
      !result->Evaluate(dict_.get(),
                        enable_user_dict ? user_dict_.get() : NULL)) {
    return nullptr;
  }
  return New<DistinctTranslation>(result);
}

string ScriptTranslator::FormatPreedit(const string& preedit) {
  string result = preedit;
  preedit_formatter_.Apply(&result);
  return result;
}

string ScriptTranslator::Spell(const Code& code) {
  string result;
  vector<string> syllables;
  if (!dict_ || !dict_->Decode(code, &syllables) || syllables.empty())
    return result;
  result =  boost::algorithm::join(syllables,
                                   string(1, delimiters_.at(0)));
  comment_formatter_.Apply(&result);
  return result;
}

bool ScriptTranslator::Memorize(const CommitEntry& commit_entry) {
  bool update_elements = false;
  // avoid updating single character entries within a phrase which is
  // composed with single characters only
  if (commit_entry.elements.size() > 1) {
    for (const DictEntry* e : commit_entry.elements) {
      if (e->code.size() > 1) {
        update_elements = true;
        break;
      }
    }
  }
  if (update_elements) {
    for (const DictEntry* e : commit_entry.elements) {
      user_dict_->UpdateEntry(*e, 0);
    }
  }
  user_dict_->UpdateEntry(commit_entry, 1);
  return true;
}

// ScriptSyllabifier implementation

Spans ScriptSyllabifier::Syllabify(const Phrase* phrase) {
  Spans result;
  vector<size_t> vertices;
  vertices.push_back(start_);
  SyllabifyTask task{
    phrase->code(),
    syllable_graph_,
    phrase->end() - start_,
    [&](SyllabifyTask* task, size_t depth,
        size_t current_pos, size_t next_pos) {
      vertices.push_back(start_ + next_pos);
    },
    [&](SyllabifyTask* task, size_t depth) {
      vertices.pop_back();
    }
  };
  if (syllabify_dfs(&task, 0, phrase->start() - start_)) {
    result.set_vertices(std::move(vertices));
  }
  return result;
}

size_t ScriptSyllabifier::BuildSyllableGraph(Prism& prism) {
  return (size_t)syllabifier_.BuildSyllableGraph(input_,
                                                 prism,
                                                 &syllable_graph_);
}

bool ScriptSyllabifier::IsCandidateCorrection(const rime::Phrase &cand) const {
  std::stack<bool> results;
  // Perform DFS on syllable graph to find whether this candidate is a correction
  SyllabifyTask task {
    cand.code(),
    syllable_graph_,
    cand.end() - start_,
    [&](SyllabifyTask* task, size_t depth,
        size_t current_pos, size_t next_pos) {
      auto id = cand.code()[depth];
      auto it_s = syllable_graph_.edges.find(current_pos);
      // C++ prohibit operator [] of const map
      // if (syllable_graph_.edges[current_pos][next_pos][id].type == kCorrection)
      if (it_s != syllable_graph_.edges.end()) {
        auto it_e = it_s->second.find(next_pos);
        if (it_e != it_s->second.end()) {
          auto it_type = it_e->second.find(id);
          if (it_type != it_e->second.end()) {
            results.push(it_type->second.is_correction);
            return;
          }
        }
      }
      results.push(false);
    },
    [&](SyllabifyTask* task, size_t depth) {
      results.pop();
    }
  };
  if (syllabify_dfs(&task, 0, cand.start() - start_)) {
    for (; !results.empty(); results.pop()) {
      if (results.top())
        return results.top();
    }
  }
  return false;
}

string ScriptSyllabifier::GetPreeditString(const Phrase& cand) const {
  const auto& delimiters = translator_->delimiters();
  std::stack<size_t> lengths;
  string output;
  SyllabifyTask task{
    cand.code(),
    syllable_graph_,
    cand.end() - start_,
    [&](SyllabifyTask* task, size_t depth,
        size_t current_pos, size_t next_pos) {
      size_t len = output.length();
      if (depth > 0 && len > 0 &&
          delimiters.find(output[len - 1]) == string::npos) {
        output += delimiters.at(0);
      }
      output += input_.substr(current_pos, next_pos - current_pos);
      lengths.push(len);
    },
    [&](SyllabifyTask* task, size_t depth) {
      output.resize(lengths.top());
      lengths.pop();
    }
  };
  if (syllabify_dfs(&task, 0, cand.start() - start_)) {
    return translator_->FormatPreedit(output);
  }
  else {
    return string();
  }
}

string ScriptSyllabifier::GetOriginalSpelling(const Phrase& cand) const {
  if (translator_ &&
      static_cast<int>(cand.code().size()) <= translator_->spelling_hints()) {
    return translator_->Spell(cand.code());
  }
  return string();
}

// ScriptTranslation implementation

bool ScriptTranslation::Evaluate(Dictionary* dict, UserDictionary* user_dict) {
  size_t consumed = syllabifier_->BuildSyllableGraph(*dict->prism());
  const auto& syllable_graph = syllabifier_->syllable_graph();

  phrase_ = dict->Lookup(syllable_graph, 0);
  if (user_dict) {
    user_phrase_ = user_dict->Lookup(syllable_graph, 0);
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
      syllable_graph.edges.size() > 1) {  // at least 2 syllables required
    sentence_ = MakeSentence(dict, user_dict);
  }

  if (phrase_)
    phrase_iter_ = phrase_->rbegin();
  if (user_phrase_)
    user_phrase_iter_ = user_phrase_->rbegin();
  return !CheckEmpty();
}

bool ScriptTranslation::Next() {
  bool is_correction;
  do {
    is_correction = false;
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
      DictEntryList& entries(user_phrase_iter_->second);
      if (++user_phrase_index_ >= entries.size()) {
        ++user_phrase_iter_;
        user_phrase_index_ = 0;
      }
    }
    else if (phrase_code_length > 0) {
      DictEntryIterator& iter(phrase_iter_->second);
      if (!iter.Next()) {
        ++phrase_iter_;
      }
    }
    if (enable_correction_) {
      PrepareCandidate();
      if (!candidate_) {
        break;
      }
      is_correction = syllabifier_->IsCandidateCorrection(*candidate_);
    }
  } while ( // limit the number of correction candidates
      enable_correction_ &&
      is_correction &&
      correction_count_ > max_corrections_);
  if (is_correction) {
    ++correction_count_;
  }
  return !CheckEmpty();
}

bool ScriptTranslation::IsNormalSpelling() const {
  const auto& syllable_graph = syllabifier_->syllable_graph();
  return !syllable_graph.vertices.empty() &&
      (syllable_graph.vertices.rbegin()->second == kNormalSpelling);
}

an<Candidate> ScriptTranslation::Peek() {
  PrepareCandidate();
  if (!candidate_) {
    return nullptr;
  }
  if (candidate_->preedit().empty()) {
    candidate_->set_preedit(syllabifier_->GetPreeditString(*candidate_));
  }
  if (candidate_->comment().empty()) {
    auto spelling = syllabifier_->GetOriginalSpelling(*candidate_);
    if (!spelling.empty() && spelling != candidate_->preedit()) {
      candidate_->set_comment(/*quote_left + */spelling/* + quote_right*/);
    }
  }
  candidate_->set_syllabifier(syllabifier_);
  return candidate_;
}

void ScriptTranslation::PrepareCandidate() {
  if (exhausted()) {
    candidate_ = nullptr;
    return;
  }
  if (sentence_) {
    if (sentence_->preedit().empty()) {
      sentence_->set_preedit(syllabifier_->GetPreeditString(*sentence_));
    }
    if (sentence_->comment().empty()) {
      auto spelling = syllabifier_->GetOriginalSpelling(*sentence_);
      if (!spelling.empty() &&
          (translator_->always_show_comments() ||
              spelling != sentence_->preedit())) {
        sentence_->set_comment(/*quote_left + */spelling/* + quote_right*/);
      }
    }
    candidate_ = sentence_;
    return;
  }
  size_t user_phrase_code_length = 0;
  if (user_phrase_ && user_phrase_iter_ != user_phrase_->rend()) {
    user_phrase_code_length = user_phrase_iter_->first;
  }
  size_t phrase_code_length = 0;
  if (phrase_ && phrase_iter_ != phrase_->rend()) {
    phrase_code_length = phrase_iter_->first;
  }
  an<Phrase> cand;
  if (user_phrase_code_length > 0 &&
      user_phrase_code_length >= phrase_code_length) {
    DictEntryList& entries(user_phrase_iter_->second);
    const auto& entry(entries[user_phrase_index_]);
    DLOG(INFO) << "user phrase '" << entry->text
               << "', code length: " << user_phrase_code_length;
    cand = New<Phrase>(translator_->language(),
                       "phrase",
                       start_,
                       start_ + user_phrase_code_length,
                       entry);
    cand->set_quality(exp(entry->weight) +
                      translator_->initial_quality() +
                      (IsNormalSpelling() ? 0.5 : -0.5));
  }
  else if (phrase_code_length > 0) {
    DictEntryIterator& iter(phrase_iter_->second);
    const auto& entry(iter.Peek());
    DLOG(INFO) << "phrase '" << entry->text
               << "', code length: " << user_phrase_code_length;
    cand = New<Phrase>(translator_->language(),
                       "phrase",
                       start_,
                       start_ + phrase_code_length,
                       entry);
    cand->set_quality(exp(entry->weight) +
                      translator_->initial_quality() +
                      (IsNormalSpelling() ? 0 : -1));
  }
  candidate_ = cand;
}

bool ScriptTranslation::CheckEmpty() {
  set_exhausted((!phrase_ || phrase_iter_ == phrase_->rend()) &&
                (!user_phrase_ || user_phrase_iter_ == user_phrase_->rend()));
  return exhausted();
}

an<Sentence> ScriptTranslation::MakeSentence(Dictionary* dict,
                                             UserDictionary* user_dict) {
  const int kMaxSyllablesForUserPhraseQuery = 5;
  const auto& syllable_graph = syllabifier_->syllable_graph();
  WordGraph graph;
  for (const auto& x : syllable_graph.edges) {
    UserDictEntryCollector& dest(graph[x.first]);
    if (user_dict) {
      auto user_phrase = user_dict->Lookup(syllable_graph, x.first,
                                           kMaxSyllablesForUserPhraseQuery);
      if (user_phrase)
        dest.swap(*user_phrase);
    }
    if (auto phrase = dict->Lookup(syllable_graph, x.first)) {
      // merge lookup results
      for (auto& y : *phrase) {
        DictEntryList& entries(dest[y.first]);
        while (entries.size() < translator_->max_homophones() &&
               !y.second.exhausted()) {
          entries.push_back(y.second.Peek());
          if (!y.second.Next())
            break;
        }
      }
    }
  }
  auto sentence = poet_->MakeSentence(graph, syllable_graph.interpreted_length);
  if (sentence) {
    sentence->Offset(start_);
    sentence->set_syllabifier(syllabifier_);
  }
  return sentence;
}

}  // namespace rime
