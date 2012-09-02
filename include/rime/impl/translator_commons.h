// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TRANSLATOR_COMMONS_H_
#define RIME_TRANSLATOR_COMMONS_H_

#include <vector>
#include <string>
#include <boost/regex.hpp>
#include <boost/signals/connection.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/candidate.h>
#include <rime/translation.h>
#include <rime/algo/algebra.h>
#include <rime/dict/vocabulary.h>

namespace rime {

//

class Patterns : public std::vector<boost::regex> {
 public:
  bool Load(ConfigListPtr patterns);
};

//

class Language;

class Phrase : public Candidate {
 public:
  Phrase(Language* language,
         const std::string& type, size_t start, size_t end,
         const shared_ptr<DictEntry> &entry)
      : Candidate(type, start, end),
        language_(language),
        entry_(entry) {
  }
  const std::string& text() const { return entry_->text; }
  const std::string comment() const { return entry_->comment; }
  const std::string preedit() const { return entry_->preedit; }
  void set_comment(const std::string &comment) {
    entry_->comment = comment;
  }
  void set_preedit(const std::string &preedit) {
    entry_->preedit = preedit;
  }
  const Code& code() const { return entry_->code; }
  const DictEntry& entry() const { return *entry_; }
  Language* language() const { return language_; }
  
 protected:
  Language* language_;
  const shared_ptr<DictEntry> entry_;
};

//

class Sentence : public Candidate {
 public:
  Sentence(Language* language)
      : Candidate("sentence", 0, 0), language_(language) {
    entry_.weight = 1.0;
  }
  void Extend(const DictEntry& entry, size_t end_pos);
  void Offset(size_t offset);

  const std::string& text() const { return entry_.text; }
  const std::string comment() const { return entry_.comment; }
  const std::string preedit() const { return entry_.preedit; }
  void set_comment(const std::string &comment) {
    entry_.comment = comment;
  }
  void set_preedit(const std::string &preedit) {
    entry_.preedit = preedit;
  }
  const Code& code() const { return entry_.code; }
  double weight() const { return entry_.weight; }
  const std::vector<DictEntry>& components() const
  { return components_; }
  const std::vector<size_t>& syllable_lengths() const
  { return syllable_lengths_; }
  
  Language* language() const { return language_; }

 protected:
  Language* language_;
  DictEntry entry_;
  std::vector<DictEntry> components_;
  std::vector<size_t> syllable_lengths_;
};

//

class CharsetFilter : public Translation {
 public:
  CharsetFilter(shared_ptr<Translation> translation_);
  virtual bool Next();
  virtual shared_ptr<Candidate> Peek();

  static bool Passed(const std::string& text);
  
 protected:
  bool LocateNextCandidate();
  
  shared_ptr<Translation> translation_;
};

//

class Context;
class Engine;
class Dictionary;
class UserDictionary;

class Language {
};

class Memory : public Language {
 public:
  Memory(Engine* engine);
  virtual ~Memory();
  
  virtual bool Memorize(const DictEntry& commit_entry,
                        const std::vector<const DictEntry*>& elements) = 0;

  // TODO
  Language* language() { return this; }
  
  Dictionary* dict() const { return dict_.get(); }
  UserDictionary* user_dict() const { return user_dict_.get(); }
  
 protected:
  virtual void OnCommit(Context* ctx);
  virtual void OnDeleteEntry(Context* ctx);

  scoped_ptr<Dictionary> dict_;
  scoped_ptr<UserDictionary> user_dict_;
  
 private:
  boost::signals::connection commit_connection_;
  boost::signals::connection delete_connection_;
};

//

class TranslatorOptions {
 public:
  TranslatorOptions(Engine* engine, const std::string& prefix = "translator");
  
  const std::string& delimiters() const { return delimiters_; }
  bool enable_completion() const { return enable_completion_; }
  Projection& preedit_formatter() { return preedit_formatter_; }
  Projection& comment_formatter() { return comment_formatter_; }
  
 protected:
  std::string delimiters_;
  bool enable_completion_;
  Projection preedit_formatter_;
  Projection comment_formatter_;
  Patterns user_dict_disabling_patterns_;
};

}  // namespace rime

#endif  // RIME_TRANSLATOR_COMMONS_H_
