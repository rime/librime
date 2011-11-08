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
            int start,
            int end) : type_(type), start_(start), end_(end) {}
  virtual ~Candidate() {}

  // recognized by translators in learning phase
  const std::string& type() const { return type_; }
  // [start, end) mark a range in the input that the candidate correspond to
  int start() const { return start_; }
  int end() const { return end_; }
  
  // candidate text to commit
  virtual const char* text() const = 0;
  // (optional)
  virtual const char* comment() const { return NULL; }
  // text shown in the preedit area, replacing input string (optional)
  virtual const char* preedit() const { return NULL; }

  void set_type(const std::string &type) { type_ = type; }
  void set_start(int start) { start_ = start; }
  void set_end(int end) { end_ = end; }

 private:
  std::string type_;
  int start_;
  int end_;
};

class SimpleCandidate : public Candidate {
 public:
  SimpleCandidate() : Candidate() {}
  SimpleCandidate(const std::string type,
                  int start,
                  int end,
                  const std::string text,
                  const std::string comment = std::string(),
                  const std::string preedit = std::string())
      : Candidate(type, start, end), text_(text) {}

  const char* text() const { return text_.c_str(); }
  const char* comment() const { return comment_.c_str(); }
  const char* preedit() const { return preedit_.c_str(); }

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
