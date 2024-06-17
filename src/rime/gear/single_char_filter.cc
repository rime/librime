//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-11-19 Chen Gong <chen.sst@gmail.com>
//
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/translation.h>
#include <rime/gear/single_char_filter.h>
#include <rime/gear/translator_commons.h>

namespace rime {

static inline size_t unistrlen(const string& text) {
  return utf8::unchecked::distance(text.c_str(), text.c_str() + text.length());
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
    if (!phrase ||
        (phrase->type() != "table" && phrase->type() != "user_table")) {
      break;
    }
    if (unistrlen(cand->text()) == 1) {
      top.push_back(cand);
    } else {
      bottom.push_back(cand);
    }
    translation_->Next();
  }
  cache_.splice(cache_.end(), top);
  cache_.splice(cache_.end(), bottom);
  return !cache_.empty();
}

class SingleCharOnlyTranslation : public Translation {
 public:
  SingleCharOnlyTranslation(an<Translation> translation);

  bool Next() override;
  an<Candidate> Peek() override;

 private:
  bool SkipToNextChar();

  an<Translation> translation_;
  an<Candidate> current_;
};

SingleCharOnlyTranslation::SingleCharOnlyTranslation(
    an<Translation> translation)
    : translation_(translation) {
  SkipToNextChar();
}

bool SingleCharOnlyTranslation::SkipToNextChar() {
  while (true) {
    if (translation_->exhausted() || !translation_->Next()) {
      set_exhausted(true);
      current_.reset();
      return false;
    }
    current_ = translation_->Peek();
    if (unistrlen(current_->text()) == 1)
      return true;
  }
}

an<Candidate> SingleCharOnlyTranslation::Peek() {
  return current_;
}

bool SingleCharOnlyTranslation::Next() {
  if (exhausted())
    return false;
  return SkipToNextChar();
}

SingleCharFilter::SingleCharFilter(const Ticket& ticket) : Filter(ticket) {
  if (name_space_ == "") {
    name_space_ = "single_char_filter";
  }
  if (Config* config = engine_->schema()->config()) {
    string type;
    if (config->GetString(name_space_ + "/type", &type)) {
      type_ = (type == "char_only") ? kCharOnly : kCharFirst;
    } else {
      type_ = kCharFirst;
    }
    config->GetString(name_space_ + "/option_name", &option_name_);
  }
}

an<Translation> SingleCharFilter::Apply(an<Translation> translation,
                                        CandidateList* candidates) {
  // for backward compatibility, always enable filter if no option_name is given
  if (option_name_ != "" && !engine_->context()->get_option(option_name_)) {
    return translation;
  }
  if (type_ == kCharFirst) {
    return New<SingleCharFirstTranslation>(translation);
  } else {
    return New<SingleCharOnlyTranslation>(translation);
  }
}

}  // namespace rime
