// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATION_H_
#define RIME_TRANSLATION_H_

#include <string>
#include <rime/candidate.h>
#include <rime/common.h>

namespace rime {

class Translation {
 public:
  Translation() : exhausted_(false) {}
  virtual ~Translation() {}

  // A translation may contain multiple results, looks
  // something like a generator of candidates.
  virtual bool Next() = 0;

  virtual shared_ptr<Candidate> Peek() = 0;

  // should it provide the next candidate (negative value, zero) or
  // should it give up the chance for other translations (positive)?
  virtual int Compare(shared_ptr<Translation> other,
                      const CandidateList &candidates);

  bool exhausted() const { return exhausted_; }

 protected:
  void set_exhausted(bool exhausted) { exhausted_ = exhausted; }

 private:
  bool exhausted_;
};

class UniqueTranslation : public Translation {
 public:
  UniqueTranslation(const shared_ptr<Candidate> &candidate)
      : candidate_(candidate) {
  }
  virtual ~UniqueTranslation() {
  }

  virtual bool Next() {
    if (exhausted())
      return false;
    set_exhausted(true);
    return true;
  }

  virtual shared_ptr<Candidate> Peek() {
    if (exhausted())
      return shared_ptr<Candidate>();
    return candidate_;
  }

 protected:
  shared_ptr<Candidate> candidate_;
};

class FifoTranslation : public Translation {
 public:
  FifoTranslation() : cursor_(0) {}

  bool Next() {
    if (exhausted())
      return false;
    if (++cursor_ >= candies_.size())
      set_exhausted(true);
    return true;
  }

  shared_ptr<Candidate> Peek() {
    if (exhausted())
      return shared_ptr<Candidate>();
    return candies_[cursor_];
  }

  void Append(const shared_ptr<Candidate> &candy) {
    candies_.push_back(candy);
    set_exhausted(false);
  }

  size_t size() const {
    return candies_.size() - cursor_;
  }

 private:
  std::vector<shared_ptr<Candidate> > candies_;
  size_t cursor_;
};

} // namespace rime

#endif  // RIME_TRANSLATION_H_
