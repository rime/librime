//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SCRIPT_TRANSLATOR_H_
#define RIME_SCRIPT_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/algo/syllabifier.h>
#include <rime/context.h>
#include <rime/gear/memory.h>
#include <rime/gear/translator_commons.h>

namespace rime {

class Code;
class Corrector;
struct DictEntry;
class Dictionary;
class Poet;
class UserDictionary;

// ConstraintFilteredTranslation: wraps a Translation and skips candidates
// whose decoded code doesn't match tab constraints (for 简拼 filtering)
class ConstraintFilteredTranslation : public CacheTranslation {
 public:
  ConstraintFilteredTranslation(an<Translation> translation,
                                Dictionary* dict,
                                const vector<Context::TabConstraint>& constraints);
  virtual bool Next() override;

 protected:
  bool MatchesConstraints(const an<Candidate>& cand);

  Dictionary* dict_;
  vector<Context::TabConstraint> constraints_;
};

class ScriptTranslator : public Translator,
                         public Memory,
                         public TranslatorOptions {
 public:
  ScriptTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                const Segment& segment) override;
  virtual bool Memorize(const CommitEntry& commit_entry) override;
  virtual bool ProcessSegmentOnCommit(CommitEntry& commit_entry,
                                      const Segment& seg) override;

  string FormatPreedit(const string& preedit);
  string Spell(const Code& code);
  string GetPrecedingText(size_t start) const;
  bool UpdateElements(const CommitEntry& commit_entry);

  bool ConcatenatePhrases(CommitEntry& commit_entry,
                          const vector<an<Phrase>>& phrases);
  bool SaveCommitEntry(CommitEntry& commit_entry);

  // options
  int max_homophones() const { return max_homophones_; }
  int spelling_hints() const { return spelling_hints_; }
  bool always_show_comments() const { return always_show_comments_; }
  bool enable_word_completion() const { return enable_word_completion_; }
  int max_word_length() const { return max_word_length_; }
  int core_word_length() const;

  //! Collect possible tab entries from the cached SyllableGraph
  RIME_DLL void CollectSyllableTabs(size_t start_pos,
                                    vector<InputTabEntry>* tabs) const;
  //! Access the cached SyllableGraph (may be empty if no Query was made)
  RIME_DLL const SyllableGraph* cached_syllable_graph() const {
    return has_cached_graph_ ? &cached_syllable_graph_ : nullptr;
  }
  //! Access the engine (for reading Context constraints)
  Engine* engine() const { return engine_; }

 protected:
  int max_homophones_ = 1;
  int spelling_hints_ = 0;
  int max_word_length_ = 0;
  int core_word_length_ = 0;
  bool always_show_comments_ = false;
  bool enable_correction_ = false;
  bool enable_word_completion_ = false;
  the<Corrector> corrector_;
  the<Poet> poet_;
  vector<an<Phrase>> queue_;

  // Cached SyllableGraph from the most recent Query()
  SyllableGraph cached_syllable_graph_;
  bool has_cached_graph_ = false;
};

}  // namespace rime

#endif  // RIME_SCRIPT_TRANSLATOR_H_
