//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/ticket.h>
#include <rime/gear/translator_commons.h>

namespace rime {

// Patterns

bool Patterns::Load(ConfigListPtr patterns) {
  clear();
  if (!patterns)
    return false;
  for (auto it = patterns->begin(); it != patterns->end(); ++it) {
    if (auto value = As<ConfigValue>(*it)) {
      push_back(boost::regex(value->str()));
    }
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

// UniqueFilter

UniqueFilter::UniqueFilter(shared_ptr<Translation> translation)
    : CacheTranslation(translation) {
}

bool UniqueFilter::Next() {
  if (exhausted())
    return false;
  // skip duplicate candidates
  do {
    candidate_set_.insert(Peek()->text());
    CacheTranslation::Next();
  }
  while (!exhausted() &&
         AlreadyHas(Peek()->text()));
  return !exhausted();
}

bool UniqueFilter::AlreadyHas(const std::string& text) const {
  return candidate_set_.find(text) != candidate_set_.end();
}

// TranslatorOptions

TranslatorOptions::TranslatorOptions(const Ticket& ticket) {
  if (!ticket.schema)
    return;
  if (Config *config = ticket.schema->config()) {
    config->GetString(ticket.name_space + "/delimiter", &delimiters_) ||
        config->GetString("speller/delimiter", &delimiters_);
    config->GetString(ticket.name_space + "/tag", &tag_);
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
        config->GetList(
            ticket.name_space + "/disable_user_dict_for_patterns"));
  }
  if (delimiters_.empty()) {
    delimiters_ = " ";
  }
}

bool TranslatorOptions::IsUserDictDisabledFor(const std::string& input) const {
  if (user_dict_disabling_patterns_.empty())
    return false;
  for (const auto& pattern : user_dict_disabling_patterns_) {
    if (boost::regex_match(input, pattern))
      return true;
  }
  return false;
}

}  // namespace rime
