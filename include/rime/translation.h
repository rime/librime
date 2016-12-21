//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATION_H_
#define RIME_TRANSLATION_H_

#include <rime/candidate.h>
#include <rime/common.h>

namespace rime {

class Translation {
 public:
  Translation() = default;
  virtual ~Translation() = default;

  // A translation may contain multiple results, looks
  // something like a generator of candidates.
  virtual bool Next() = 0;

  virtual an<Candidate> Peek() = 0;

  // should it provide the next candidate (negative value, zero) or
  // should it give up the chance for other translations (positive)?
  virtual int Compare(an<Translation> other,
                      const CandidateList& candidates);

  bool exhausted() const { return exhausted_; }

 protected:
  void set_exhausted(bool exhausted) { exhausted_ = exhausted; }

 private:
  bool exhausted_ = false;
};

class UniqueTranslation : public Translation {
 public:
  UniqueTranslation(an<Candidate> candidate)
      : candidate_(candidate) {
    set_exhausted(!candidate);
  }

  bool Next();
  an<Candidate> Peek();

 protected:
  an<Candidate> candidate_;
};

class FifoTranslation : public Translation {
 public:
  FifoTranslation();

  bool Next();
  an<Candidate> Peek();

  void Append(an<Candidate> candy);

  size_t size() const {
    return candies_.size() - cursor_;
  }

 protected:
  CandidateList candies_;
  size_t cursor_ = 0;
};

class UnionTranslation : public Translation {
 public:
  UnionTranslation();

  bool Next();
  an<Candidate> Peek();

  UnionTranslation& operator+= (an<Translation> t);

 protected:
  list<of<Translation>> translations_;
};

an<UnionTranslation> operator+ (an<Translation> x, an<Translation> y);

class MergedTranslation : public Translation {
 public:
  explicit MergedTranslation(const CandidateList& previous_candidates);

  bool Next();
  an<Candidate> Peek();

  MergedTranslation& operator+= (an<Translation> t);

  size_t size() const { return translations_.size(); }

 protected:
  void Elect();

  const CandidateList& previous_candidates_;
  vector<of<Translation>> translations_;
  size_t elected_ = 0;
};

class CacheTranslation : public Translation {
 public:
  CacheTranslation(an<Translation> translation);

  virtual bool Next();
  virtual an<Candidate> Peek();

 protected:
  an<Translation> translation_;
  an<Candidate> cache_;
};

template <class T, class... Args>
inline an<Translation> Cached(Args&&... args) {
  return New<CacheTranslation>(New<T>(std::forward<Args>(args)...));
}

class DistinctTranslation : public CacheTranslation {
 public:
  DistinctTranslation(an<Translation> translation);
  virtual bool Next();

 protected:
  bool AlreadyHas(const string& text) const;

  set<string> candidate_set_;
};

class PrefetchTranslation : public Translation {
 public:
  PrefetchTranslation(an<Translation> translation);

  virtual bool Next();
  virtual an<Candidate> Peek();

 protected:
  virtual bool Replenish() { return false; }

  an<Translation> translation_;
  CandidateQueue cache_;
};

} // namespace rime

#endif  // RIME_TRANSLATION_H_
