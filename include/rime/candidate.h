//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-22 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CANDIDATE_H_
#define RIME_CANDIDATE_H_

#include <rime/common.h>

namespace rime {

class Candidate {
 public:
  Candidate() = default;
  Candidate(const string type,
            size_t start,
            size_t end,
            double quality = 0.)
      : type_(type), start_(start), end_(end), quality_(quality) {}
  virtual ~Candidate() = default;

  static an<Candidate>
  GetGenuineCandidate(const an<Candidate>& cand);
  static vector<of<Candidate>>
  GetGenuineCandidates(const an<Candidate>& cand);

  // recognized by translators in learning phase
  const string& type() const { return type_; }
  // [start, end) mark a range in the input that the candidate corresponds to
  size_t start() const { return start_; }
  size_t end() const { return end_; }
  double quality() const { return quality_; }

  // candidate text to commit
  virtual const string& text() const = 0;
  // (optional)
  virtual string comment() const { return string(); }
  // text shown in the preedit area, replacing input string (optional)
  virtual string preedit() const { return string(); }

  void set_type(const string& type) { type_ = type; }
  void set_start(size_t start) { start_ = start; }
  void set_end(size_t end) { end_ = end; }
  void set_quality(double quality) { quality_ = quality; }

 private:
  string type_;
  size_t start_ = 0;
  size_t end_ = 0;
  double quality_ = 0.;
};

using CandidateQueue = list<of<Candidate>>;
using CandidateList = vector<of<Candidate>>;

// useful implimentations

class SimpleCandidate : public Candidate {
 public:
  SimpleCandidate() = default;
  SimpleCandidate(const string type,
                  size_t start,
                  size_t end,
                  const string& text,
                  const string& comment = string(),
                  const string& preedit = string())
      : Candidate(type, start, end),
      text_(text), comment_(comment), preedit_(preedit) {}

  const string& text() const { return text_; }
  string comment() const { return comment_; }
  string preedit() const { return preedit_; }

  void set_text(const string& text) { text_ = text; }
  void set_comment(const string& comment) { comment_ = comment; }
  void set_preedit(const string& preedit) { preedit_ = preedit; }

 protected:
  string text_;
  string comment_;
  string preedit_;
};

class ShadowCandidate : public Candidate {
 public:
  ShadowCandidate(const an<Candidate>& item,
                  const string& type,
                  const string& text = string(),
                  const string& comment = string())
      : Candidate(type, item->start(), item->end(), item->quality()),
        text_(text), comment_(comment),
        item_(item) {}

  const string& text() const {
    return text_.empty() ? item_->text() : text_;
  }
  string comment() const {
    return comment_.empty() ? item_->comment() : comment_;
  }
  string preedit() const {
    return item_->preedit();
  }

  const an<Candidate>& item() const { return item_; }

 protected:
  string text_;
  string comment_;
  an<Candidate> item_;
};

class UniquifiedCandidate : public Candidate {
 public:
  UniquifiedCandidate(const an<Candidate>& item,
                      const string& type,
                      const string& text = string(),
                      const string& comment = string())
      : Candidate(type, item->start(), item->end(), item->quality()),
        text_(text), comment_(comment) {
    Append(item);
  }

  const string& text() const {
    return text_.empty() && !items_.empty() ?
        items_.front()->text() : text_;
  }
  string comment() const {
    return comment_.empty() && !items_.empty() ?
        items_.front()->comment() : comment_;
  }
  string preedit() const {
    return !items_.empty() ? items_.front()->preedit() : string();
  }

  void Append(an<Candidate> item) {
    items_.push_back(item);
    if (quality() < item->quality())
      set_quality(item->quality());
  }
  const CandidateList& items() const { return items_; }

 protected:
  string text_;
  string comment_;
  CandidateList items_;
};

}  // namespace rime

#endif  // RIME_CANDIDATE_H_
