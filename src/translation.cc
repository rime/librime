//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-05-21 GONG Chen <chen.sst@gmail.com>
//
#include <rime/candidate.h>
#include <rime/translation.h>

namespace rime {

int Translation::Compare(shared_ptr<Translation> other,
                         const CandidateList &candidates) {
  if (!other || other->exhausted()) return -1;
  if (exhausted()) return 1;
  shared_ptr<const Candidate> ours = Peek();
  shared_ptr<const Candidate> theirs = other->Peek();
  if (!ours) return 1;
  if (!theirs) return -1;
  int k = 0;
  // the one nearer to the beginning of segment comes first
  k = ours->start() - theirs->start();
  if (k != 0) return k;
  // then the longer comes first
  k = ours->end() - theirs->end();
  if (k != 0) return -k;
  // compare quality
  double qdiff = ours->quality() - theirs->quality();
  if (qdiff != 0.)
    return (qdiff > 0.) ? -1 : 1;
  // draw
  return 0;
}

bool UniqueTranslation::Next() {
  if (exhausted())
    return false;
  set_exhausted(true);
  return true;
}

shared_ptr<Candidate> UniqueTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  return candidate_;
}

FifoTranslation::FifoTranslation() : cursor_(0) {
  set_exhausted(true);
}

bool FifoTranslation::Next() {
  if (exhausted())
    return false;
  if (++cursor_ >= candies_.size())
    set_exhausted(true);
  return true;
}

shared_ptr<Candidate> FifoTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  return candies_[cursor_];
}

void FifoTranslation::Append(const shared_ptr<Candidate>& candy) {
  candies_.push_back(candy);
  set_exhausted(false);
}

UnionTranslation::UnionTranslation() {
  set_exhausted(true);
}

bool UnionTranslation::Next() {
  if (exhausted())
    return false;
  translations_.front()->Next();
  if (translations_.front()->exhausted()) {
    translations_.pop_front();
    if (translations_.empty()) {
      set_exhausted(true);
    }
  }
  return true;
}

shared_ptr<Candidate> UnionTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  return translations_.front()->Peek();
}

UnionTranslation& UnionTranslation::operator+= (shared_ptr<Translation> t) {
  if (t && !t->exhausted()) {
    translations_.push_back(t);
    set_exhausted(false);
  }
  return *this;
}

shared_ptr<UnionTranslation> operator+ (shared_ptr<Translation> a,
                                        shared_ptr<Translation> b) {
  auto c = New<UnionTranslation>();
  *c += a;
  *c += b;
  return c->exhausted() ? shared_ptr<UnionTranslation>() : c;
}

}  // namespace rime
