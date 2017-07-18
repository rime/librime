//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
//
#include <stdint.h> // for uint32_t
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/dict/vocabulary.h>
#include <rime/gear/charset_filter.h>
#include <boost/locale/encoding.hpp>

namespace rime {

bool is_extended_cjk(uint32_t ch)
{
  if ((ch >= 0x3400 && ch <= 0x4DBF) ||    // CJK Unified Ideographs Extension A
      (ch >= 0x20000 && ch <= 0x2A6DF) ||  // CJK Unified Ideographs Extension B
      (ch >= 0x2A700 && ch <= 0x2B73F) ||  // CJK Unified Ideographs Extension C
      (ch >= 0x2B740 && ch <= 0x2B81F) ||  // CJK Unified Ideographs Extension D
      (ch >= 0x2B820 && ch <= 0x2CEAF) ||  // CJK Unified Ideographs Extension E
      (ch >= 0x2CEB0 && ch <= 0x2EBEF) ||  // CJK Unified Ideographs Extension F
      (ch >= 0x2F800 && ch <= 0x2FA1F))    // CJK Compatibility Ideographs Supplement
    return true;

  return false;
}

bool contains_extended_cjk(const string& text)
{
  const char *p = text.c_str();
  uint32_t ch;

  while ((ch = utf8::unchecked::next(p)) != 0) {
    if (is_extended_cjk(ch)) {
      return true;
    }
  }

  return false;
}

// CharsetFilterTranslation

CharsetFilterTranslation::CharsetFilterTranslation(
    an<Translation> translation, const string& charset)
    : translation_(translation), charset_(charset) {
  LocateNextCandidate();
}

bool CharsetFilterTranslation::Next() {
  if (exhausted())
    return false;
  if (!translation_->Next()) {
    set_exhausted(true);
    return false;
  }
  return LocateNextCandidate();
}

an<Candidate> CharsetFilterTranslation::Peek() {
  return translation_->Peek();
}

bool CharsetFilterTranslation::LocateNextCandidate() {
  while (!translation_->exhausted()) {
    auto cand = translation_->Peek();
    if (cand && CharsetFilter::FilterText(cand->text(), charset_))
      return true;
    translation_->Next();
  }
  set_exhausted(true);
  return false;
}

// CharsetFilter

bool CharsetFilter::FilterText(const string& text, const string& charset) {
  if (charset.empty()) return !contains_extended_cjk(text);
  try {
    boost::locale::conv::from_utf(text, charset, boost::locale::conv::method_type::stop);
  }
  catch(boost::locale::conv::conversion_error const& /*ex*/) {
    return false;
  }
  catch(...) {
  }
  return true;
}

bool CharsetFilter::FilterDictEntry(an<DictEntry> entry) {
  return entry && FilterText(entry->text);
}

CharsetFilter::CharsetFilter(const Ticket& ticket)
    : Filter(ticket), TagMatching(ticket) {
}

an<Translation> CharsetFilter::Apply(
    an<Translation> translation, CandidateList* candidates) {
  if (name_space_.empty() && !engine_->context()->get_option("extended_charset")) {
    return New<CharsetFilterTranslation>(translation);
  }
  if (!name_space_.empty() && engine_->context()->get_option(name_space_)) {
    return New<CharsetFilterTranslation>(translation, name_space_);
  }
  return translation;
}

}  // namespace rime
