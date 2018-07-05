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

bool is_emoji(uint32_t ch)
{

  if ((ch >= 0x0000 && ch <= 0x007F) || // C0 Controls and Basic Latin
	  (ch >= 0x0080 && ch <= 0x00FF) || // C1 Controls and Latin-1 Supplement
	  (ch >= 0x02B0 && ch <= 0x02FF) || // Spacing Modifier Letters
	  (ch >= 0x0900 && ch <= 0x097F) || // Devanagari
	  (ch >= 0x2000 && ch <= 0x203C) || // General Punctuation
	  (ch >= 0x20A0 && ch <= 0x20CF) || // Currency Symbols
	  (ch >= 0x2100 && ch <= 0x214F) || // Letterlike Symbols
	  (ch >= 0x2150 && ch <= 0x218F) || // Number Forms
	  (ch >= 0x2190 && ch <= 0x21FF) || // Arrows
	  (ch >= 0x2200 && ch <= 0x22FF) || // Mathematical Operators
	  (ch >= 0x2300 && ch <= 0x23FF) || // Miscellaneous Technical
	  (ch >= 0x2460 && ch <= 0x24FF) || // Enclosed Alphanumerics
	  (ch >= 0x25A0 && ch <= 0x25FF) || // Geometric Shapes
	  (ch >= 0x2600 && ch <= 0x26FF) || // Miscellaneous Symbols
	  (ch >= 0x2700 && ch <= 0x27BF) || // Dingbats
	  (ch >= 0x2900 && ch <= 0x297F) || // Supplemental Arrows-B
	  (ch >= 0x2B00 && ch <= 0x2BFF) || // Miscellaneous Symbols and Arrows
	  (ch >= 0x3000 && ch <= 0x303F) || // CJK Symbols and Punctuation
	  (ch >= 0x3200 && ch <= 0x32FF) || // Enclosed CJK Letters and Months
	  (ch >= 0x1F100 && ch <= 0x1F1FF) || // Enclosed Alphanumeric Supplement
	  (ch >= 0x1F200 && ch <= 0x1F2FF) || // Enclosed Ideographic Supplement
	  (ch >= 0x1F000 && ch <= 0x1F02F) || // Mahjong Tiles
	  (ch >= 0x1F0A0 && ch <= 0x1F0FF) || // Playing Cards
	  (ch >= 0x1F300 && ch <= 0x1F5FF) || // Miscellaneous Symbols and Pictographs
	  (ch >= 0x1F600 && ch <= 0x1F64F) || // Emoticons
	  (ch >= 0x1F680 && ch <= 0x1F6FF) || // Transport and Map Symbols
	  (ch >= 0x1F900 && ch <= 0x1F9FF)) // Supplemental Symbols and Pictographs)
	return true;

  return false;
}

bool contains_emoji(const string& text)
{
  const char *p = text.c_str();
  uint32_t ch;

  while ((ch = utf8::unchecked::next(p)) != 0) {
    if (is_emoji(ch)) {
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
  if (contains_emoji(text)) return true;
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
