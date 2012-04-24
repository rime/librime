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
#include <rime/common.h>
#include <rime/config.h>
#include <rime/candidate.h>
#include <rime/dict/vocabulary.h>

namespace rime {

class Patterns : public std::vector<boost::regex> {
 public:
  bool Load(ConfigListPtr patterns);
};

class ZhCandidate : public Candidate {
 public:
  ZhCandidate(size_t start, size_t end,
              const shared_ptr<DictEntry> &entry)
      : Candidate("zh", start, end),
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
 protected:
  const shared_ptr<DictEntry> entry_;
};

class Sentence : public Candidate {
 public:
  Sentence() : Candidate("sentence", 0, 0) {
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
  const std::vector<DictEntry>& components() const { return components_; }
  
 protected:
  DictEntry entry_;
  std::vector<DictEntry> components_;
};

}  // namespace rime

#endif  // RIME_TRANSLATOR_COMMONS_H_
