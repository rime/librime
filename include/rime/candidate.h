//
// Copyright RIME Developers
// Distributed under the BSD License
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
  Candidate() = default;
  Candidate(const std::string type,
            size_t start,
            size_t end,
            double quality = 0.)
      : type_(type), start_(start), end_(end), quality_(quality) {}
  virtual ~Candidate() = default;

  static shared_ptr<Candidate>
  GetGenuineCandidate(const shared_ptr<Candidate>& cand);
  static std::vector<shared_ptr<Candidate>>
  GetGenuineCandidates(const shared_ptr<Candidate>& cand);

  // recognized by translators in learning phase
  const std::string& type() const { return type_; }
  // [start, end) mark a range in the input that the candidate corresponds to
  size_t start() const { return start_; }
  size_t end() const { return end_; }
  double quality() const { return quality_; }

  // candidate text to commit
  virtual const std::string& text() const = 0;
  // (optional)
  virtual std::string comment() const { return std::string(); }
  // text shown in the preedit area, replacing input string (optional)
  virtual std::string preedit() const { return std::string(); }

  void set_type(const std::string& type) { type_ = type; }
  void set_start(size_t start) { start_ = start; }
  void set_end(size_t end) { end_ = end; }
  void set_quality(double quality) { quality_ = quality; }

 private:
  std::string type_;
  size_t start_ = 0;
  size_t end_ = 0;
  double quality_ = 0.;
};

using CandidateList = std::vector<shared_ptr<Candidate>>;

// useful implimentations

class SimpleCandidate : public Candidate {
 public:
  SimpleCandidate() = default;
  SimpleCandidate(const std::string type,
                  size_t start,
                  size_t end,
                  const std::string& text,
                  const std::string& comment = std::string(),
                  const std::string& preedit = std::string())
      : Candidate(type, start, end),
      text_(text), comment_(comment), preedit_(preedit) {}

  const std::string& text() const { return text_; }
  std::string comment() const { return comment_; }
  std::string preedit() const { return preedit_; }

  void set_text(const std::string& text) { text_ = text; }
  void set_comment(const std::string& comment) { comment_ = comment; }
  void set_preedit(const std::string& preedit) { preedit_ = preedit; }

 protected:
  std::string text_;
  std::string comment_;
  std::string preedit_;
};

class ShadowCandidate : public Candidate {
 public:
  ShadowCandidate(const shared_ptr<Candidate>& item,
                  const std::string& type,
                  const std::string& text = std::string(),
                  const std::string& comment = std::string())
      : Candidate(type, item->start(), item->end(), item->quality()),
        text_(text), comment_(comment),
        item_(item) {}

  const std::string& text() const {
    return text_.empty() ? item_->text() : text_;
  }
  std::string comment() const {
    return comment_.empty() ? item_->comment() : comment_;
  }
  std::string preedit() const {
    return item_->preedit();
  }

  const shared_ptr<Candidate>& item() const { return item_; }

 protected:
  std::string text_;
  std::string comment_;
  shared_ptr<Candidate> item_;
};

class UniquifiedCandidate : public Candidate {
 public:
  UniquifiedCandidate(const shared_ptr<Candidate>& item,
                      const std::string& type,
                      const std::string& text = std::string(),
                      const std::string& comment = std::string())
      : Candidate(type, item->start(), item->end(), item->quality()),
        text_(text), comment_(comment) {
    Append(item);
  }

  const std::string& text() const {
    return text_.empty() && !items_.empty() ?
        items_.front()->text() : text_;
  }
  std::string comment() const {
    return comment_.empty() && !items_.empty() ?
        items_.front()->comment() : comment_;
  }
  std::string preedit() const {
    return !items_.empty() ? items_.front()->preedit() : std::string();
  }

  void Append(shared_ptr<Candidate> item) {
    items_.push_back(item);
    if (quality() < item->quality())
      set_quality(item->quality());
  }
  const CandidateList& items() const { return items_; }

 protected:
  std::string text_;
  std::string comment_;
  CandidateList items_;
};

}  // namespace rime

#endif  // RIME_CANDIDATE_H_
