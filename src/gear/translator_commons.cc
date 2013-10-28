//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <utf8.h>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/ticket.h>
#include <rime/gear/translator_commons.h>

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
  entry_->code.insert(entry_->code.end(),
                     entry.code.begin(), entry.code.end());
  entry_->text.append(entry.text);
  entry_->weight *= (std::max)(entry.weight, kEpsilon) * kPenalty;
  components_.push_back(entry);
  syllable_lengths_.push_back(end_pos - end());
  set_end(end_pos);
  DLOG(INFO) << "extend sentence " << end_pos << ") "
             << entry_->text << " : " << entry_->weight;
}

void Sentence::Offset(size_t offset) {
  set_start(start() + offset);
  set_end(end() + offset);
}

// CharsetFilter

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
    if (cand && FilterText(cand->text()))
      return true;
    translation_->Next();
  }
  set_exhausted(true);
  return false;
}

bool CharsetFilter::FilterText(const std::string& text) {
  const char* p = text.c_str();
  utf8::uint32_t c;
  while ((c = utf8::unchecked::next(p))) {
    if ((c >= 0x3400 && c <= 0x4DBF) ||    // CJK Unified Ideographs Extension A
        (c >= 0x20000 && c <= 0x2A6DF) ||  // CJK Unified Ideographs Extension B
        (c >= 0x2A700 && c <= 0x2B73F) ||  // CJK Unified Ideographs Extension C
        (c >= 0x2B840 && c <= 0x2B81F))    // CJK Unified Ideographs Extension D
      return false;
  }
  return true;
}

bool CharsetFilter::FilterDictEntry(shared_ptr<DictEntry> entry) {
  return entry && FilterText(entry->text);
}

// UniqueFilter

UniqueFilter::UniqueFilter(shared_ptr<Translation> translation)
    : translation_(translation) {
  set_exhausted(!translation_ || translation_->exhausted());
}

bool UniqueFilter::Next() {
  if (exhausted())
    return false;
  // skip duplicate candidates
  do {
    candidate_set_.insert(translation_->Peek()->text());
    translation_->Next();
  }
  while (!translation_->exhausted() &&
         AlreadyHas(translation_->Peek()->text()));
  if (translation_->exhausted()) {
    set_exhausted(true);
    return false;
  }
  return true;
}

shared_ptr<Candidate> UniqueFilter::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  return translation_->Peek();
}

bool UniqueFilter::AlreadyHas(const std::string& text) const {
  return candidate_set_.find(text) != candidate_set_.end();
}

// TranslatorOptions

TranslatorOptions::TranslatorOptions(const Ticket& ticket)
    : enable_completion_(true),
      strict_spelling_(false),
      initial_quality_(0.0) {
  if (!ticket.schema) return;
  Config *config = ticket.schema->config();
  if (config) {
    config->GetString("speller/delimiter", &delimiters_);
    config->GetBool(ticket.name_space + "/enable_completion",
                    &enable_completion_);
    config->GetBool(ticket.name_space + "/strict_spelling",
                    &strict_spelling_);
    config->GetDouble(ticket.name_space + "/initial_quality",
                      &initial_quality_);
    preedit_formatter_.Load(
        config->GetList(ticket.name_space + "/preedit_format"));
    comment_formatter_.Load(
        config->GetList(ticket.name_space + "/comment_format"));
    user_dict_disabling_patterns_.Load(
        config->GetList(ticket.name_space + "/disable_user_dict_for_patterns"));
  }
  if (delimiters_.empty()) {
    delimiters_ = " ";
  }
}

bool TranslatorOptions::IsUserDictDisabledFor(const std::string& input) const {
  if (user_dict_disabling_patterns_.empty())
    return false;
  BOOST_FOREACH(const boost::regex& pattern, user_dict_disabling_patterns_) {
    if (boost::regex_match(input, pattern))
      return true;
  }
  return false;
}


}  // namespace rime
