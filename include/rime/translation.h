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

  virtual a<Candidate> Peek() = 0;

  // should it provide the next candidate (negative value, zero) or
  // should it give up the chance for other translations (positive)?
  virtual int Compare(a<Translation> other,
                      const CandidateList& candidates);

  bool exhausted() const { return exhausted_; }

 protected:
  void set_exhausted(bool exhausted) { exhausted_ = exhausted; }

 private:
  bool exhausted_ = false;
};

class UniqueTranslation : public Translation {
 public:
  UniqueTranslation(a<Candidate> candidate)
      : candidate_(candidate) {
  }

  bool Next();
  a<Candidate> Peek();

 protected:
  a<Candidate> candidate_;
};

class FifoTranslation : public Translation {
 public:
  FifoTranslation();

  bool Next();
  a<Candidate> Peek();

  void Append(a<Candidate> candy);

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
  a<Candidate> Peek();

  UnionTranslation& operator+= (a<Translation> t);

 protected:
  list<a<Translation>> translations_;
};

a<UnionTranslation> operator+ (a<Translation> x, a<Translation> y);

class MergedTranslation : public Translation {
 public:
  explicit MergedTranslation(const CandidateList& previous_candidates);

  bool Next();
  a<Candidate> Peek();

  MergedTranslation& operator+= (a<Translation> t);

  size_t size() const { return translations_.size(); }

 protected:
  void Elect();

  const CandidateList& previous_candidates_;
  vector<a<Translation>> translations_;
  size_t elected_ = 0;
};

class CacheTranslation : public Translation {
 public:
  CacheTranslation(a<Translation> translation);

  virtual bool Next();
  virtual a<Candidate> Peek();

 protected:
  a<Translation> translation_;
  a<Candidate> cache_;
};

template <class T, class... Args>
inline a<Translation> Cached(Args&&... args) {
  return New<CacheTranslation>(New<T>(std::forward<Args>(args)...));
}

class DistinctTranslation : public CacheTranslation {
 public:
  DistinctTranslation(a<Translation> translation);
  virtual bool Next();

 protected:
  bool AlreadyHas(const string& text) const;

  set<string> candidate_set_;
};

class PrefetchTranslation : public Translation {
 public:
  PrefetchTranslation(a<Translation> translation);

  virtual bool Next();
  virtual a<Candidate> Peek();

 protected:
  virtual bool Replenish() { return false; }

  a<Translation> translation_;
  CandidateQueue cache_;
};

} // namespace rime

#endif  // RIME_TRANSLATION_H_
