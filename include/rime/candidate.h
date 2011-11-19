// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-22 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CANDIDATE_H_
#define RIME_CANDIDATE_H_

#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

class Candidate {
 public:
  Candidate() : type_(), start_(0), end_(0) {}
  Candidate(const std::string type,
            size_t start,
            size_t end) : type_(type), start_(start), end_(end) {}
  virtual ~Candidate() {}

  // recognized by translators in learning phase
  const std::string& type() const { return type_; }
  // [start, end) mark a range in the input that the candidate correspond to
  size_t start() const { return start_; }
  size_t end() const { return end_; }
  
  // candidate text to commit
  virtual const std::string& text() const = 0;
  // (optional)
  virtual const std::string comment() const { return std::string(); }
  // text shown in the preedit area, replacing input string (optional)
  virtual const std::string preedit() const { return std::string(); }

  void set_type(const std::string &type) { type_ = type; }
  void set_start(size_t start) { start_ = start; }
  void set_end(size_t end) { end_ = end; }

 private:
  std::string type_;
  size_t start_;
  size_t end_;
};

class SimpleCandidate : public Candidate {
 public:
  SimpleCandidate() : Candidate() {}
  SimpleCandidate(const std::string type,
                  size_t start,
                  size_t end,
                  const std::string &text,
                  const std::string &comment = std::string(),
                  const std::string &preedit = std::string())
      : Candidate(type, start, end),
      text_(text), comment_(comment), preedit_(preedit) {}

  const std::string& text() const { return text_; }
  const std::string comment() const { return comment_; }
  const std::string preedit() const { return preedit_; }

  void set_text(const std::string &text) { text_ = text; }
  void set_comment(const std::string &comment) { comment_ = comment; }
  void set_preedit(const std::string &preedit) { preedit_ = preedit; }

 private:
  std::string text_;
  std::string comment_;
  std::string preedit_;
};

typedef std::vector<shared_ptr<Candidate> > CandidateList;

}  // namespace rime

#endif  // RIME_CANDIDATE_H_
