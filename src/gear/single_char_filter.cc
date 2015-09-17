//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-11-19 Chen Gong <chen.sst@gmail.com>
//
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/translation.h>
#include <rime/gear/single_char_filter.h>
#include <rime/gear/translator_commons.h>

namespace rime {

static inline size_t unistrlen(const string& text) {
  return utf8::unchecked::distance(
      text.c_str(), text.c_str() + text.length());
}

class SingleCharFirstTranslation : public PrefetchTranslation {
 public:
  SingleCharFirstTranslation(an<Translation> translation);

 private:
  bool Rearrange();
};

SingleCharFirstTranslation::SingleCharFirstTranslation(
    an<Translation> translation)
    : PrefetchTranslation(translation) {
  Rearrange();
}

bool SingleCharFirstTranslation::Rearrange() {
  if (exhausted()) {
    return false;
  }
  CandidateQueue top;
  CandidateQueue bottom;
  while (!translation_->exhausted()) {
    auto cand = translation_->Peek();
    auto phrase = As<Phrase>(Candidate::GetGenuineCandidate(cand));
    if (!phrase || phrase->type() != "table") {
      break;
    }
    if (unistrlen(cand->text()) == 1) {
      top.push_back(cand);
    }
    else {
      bottom.push_back(cand);
    }
    translation_->Next();
  }
  cache_.splice(cache_.end(), top);
  cache_.splice(cache_.end(), bottom);
  return !cache_.empty();
}

SingleCharFilter::SingleCharFilter(const Ticket& ticket)
    : Filter(ticket) {
}

an<Translation> SingleCharFilter::Apply(
    an<Translation> translation, CandidateList* candidates) {
  return New<SingleCharFirstTranslation>(translation);
}

}  // namespace rime
