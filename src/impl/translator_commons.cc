// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <utf8.h>
#include <rime/config.h>
#include <rime/impl/translator_commons.h>

namespace rime {

// Patterns

bool Patterns::Load(ConfigListPtr patterns) {
  clear();
  if (!patterns) return false;
  for (ConfigList::Iterator it = patterns->begin(); it != patterns->end(); ++it) {
    ConfigValuePtr value = As<ConfigValue>(*it);
    if (!value) continue;
    push_back(boost::regex(value->str()));
  }
  return true;
}

// Sentence

void Sentence::Extend(const DictEntry& entry, size_t end_pos) {
  const double kEpsilon = 1e-200;
  const double kPenalty = 1e-8;
  entry_.code.insert(entry_.code.end(),
                     entry.code.begin(), entry.code.end());
  entry_.text.append(entry.text);
  entry_.weight *= (std::max)(entry.weight, kEpsilon) * kPenalty;
  components_.push_back(entry);
  syllable_lengths_.push_back(end_pos - end());
  set_end(end_pos);
  EZDBGONLYLOGGERPRINT("%d) %s : %g", end_pos,
                       entry_.text.c_str(), entry_.weight);
}

void Sentence::Offset(size_t offset) {
  set_start(start() + offset);
  set_end(end() + offset);
}

// TableTranslation

TableTranslation::TableTranslation(const std::string &input,
                                   size_t start, size_t end,
                                   const std::string &preedit,
                                   Projection *comment_formatter)
    : input_(input), start_(start), end_(end),
      preedit_(preedit), comment_formatter_(comment_formatter) {
  set_exhausted(true);
}

TableTranslation::TableTranslation(const DictEntryIterator& iter,
                                   const std::string &input,
                                   size_t start, size_t end,
                                   const std::string &preedit,
                                   Projection *comment_formatter)
    : iter_(iter), input_(input), start_(start), end_(end),
      preedit_(preedit), comment_formatter_(comment_formatter) {
  set_exhausted(iter_.exhausted());
}

bool TableTranslation::Next() {
  if (exhausted())
    return false;
  iter_.Next();
  set_exhausted(iter_.exhausted());
  return true;
}

shared_ptr<Candidate> TableTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e(iter_.Peek());
  std::string comment(e->comment);
  if (comment_formatter_) {
    comment_formatter_->Apply(&comment);
  }
  return boost::make_shared<SimpleCandidate>(
      e->remaining_code_length == 0 ? "zh" : "completion",
      start_,
      end_,
      e->text,
      comment,
      preedit_);
}

CharsetFilter::CharsetFilter(shared_ptr<Translation> translation)
    : translation_(translation) {
  LocateNextCandidate();
}

bool CharsetFilter::Next() {
  if (exhausted())
    return false;
  if (!translation_->Next()) {
    set_exhausted(true);
    return false;
  }
  return LocateNextCandidate();
}

shared_ptr<Candidate> CharsetFilter::Peek() {
  return translation_->Peek();
}

bool CharsetFilter::LocateNextCandidate() {
  while (!translation_->exhausted()) {
    shared_ptr<Candidate> cand = translation_->Peek();
    if (cand && Passed(cand->text()))
      return true;
    translation_->Next();
  }
  set_exhausted(true);
  return false;
}

bool CharsetFilter::Passed(const std::string& text) {
  const char* p = text.c_str();
  utf8::uint32_t c;
  while ((c = utf8::unchecked::next(p))) {
    if (c >= 0x3400 && c <= 0x4DBF ||    // CJK Unified Ideographs Extension A
        c >= 0x20000 && c <= 0x2A6DF ||  // CJK Unified Ideographs Extension B
        c >= 0x2A700 && c <= 0x2B73F ||  // CJK Unified Ideographs Extension C
        c >= 0x2B840 && c <= 0x2B81F)    // CJK Unified Ideographs Extension D
      return false;
  }
  return true;
}

}  // namespace rime
