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
#include <rime/common.h>

namespace rime {

class Candidate;

class Translation {
 public:
  Translation() : exhausted_(false) {}
  virtual ~Translation() {}

  // A translation may contain multiple results, looks
  // something like a generator of candidates.
  virtual bool Next() = 0;

  virtual shared_ptr<Candidate> Peek() = 0;

  // should it provide the next candidate (negative value) or
  // should it give the chance to other translations (positive)?
  virtual int Compare(Translation &other);

  bool exhausted() const { return exhausted_; }

 protected:
  void set_exhausted(bool exhausted) { exhausted_ = exhausted; }

 private:
  bool exhausted_;
};

class UniqueTranslation : public Translation {
 public:
  UniqueTranslation(shared_ptr<Candidate> candidate)
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

 private:
  shared_ptr<Candidate> candidate_;
};

} // namespace rime

#endif  // RIME_TRANSLATION_H_
