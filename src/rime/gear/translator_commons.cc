//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <rime/config.h>
#include <rime/schema.h>
#include <rime/ticket.h>
#include <rime/gear/grammar.h>
#include <rime/gear/translator_commons.h>

namespace rime {

// Patterns

bool Patterns::Load(an<ConfigList> patterns) {
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

// Spans

void Spans::AddVertex(size_t vertex) {
  if (vertices_.empty() || vertices_.back() < vertex) {
    vertices_.push_back(vertex);
    return;
  }
  auto lb = std::lower_bound(vertices_.begin(), vertices_.end(), vertex);
  if (*lb != vertex) {
    vertices_.insert(lb, vertex);
  }
}

void Spans::AddSpan(size_t start, size_t end) {
  AddVertex(start);
  AddVertex(end);
}

void Spans::AddSpans(const Spans& spans) {
  for (auto vertex : spans.vertices_) {
    AddVertex(vertex);
  }
}

void Spans::Clear() {
  vertices_.clear();
}

size_t Spans::PreviousStop(size_t caret_pos) const {
  for (auto x : boost::adaptors::reverse(vertices_)) {
    if (x < caret_pos)
      return x;
  }
  return caret_pos;
}

size_t Spans::NextStop(size_t caret_pos) const {
  for (auto x : vertices_) {
    if (x > caret_pos)
      return x;
  }
  return caret_pos;
}

size_t Spans::Count(size_t start_pos, size_t end_pos) const {
  size_t count = 0;
  for (auto v : vertices_) {
    if (v <= start_pos) continue;
    else if (v > end_pos) break;
    else ++count;
  }
  return count;
}

bool Spans::HasVertex(size_t vertex) const {
  return std::binary_search(vertices_.begin(), vertices_.end(), vertex);
}

bool BueKamLomaji(const string& text) {
  const string pi[] = {
    "a", "e", "i", "o", "u",
    "A", "E", "I", "O", "U",
    "á", "à", "â", "ǎ", "ā", "a", "̍", "a",
    "̋", "é", "è", "ê", "ě", "ē", "e", "̍", "e",
    "̋", "í", "ì", "î", "ǐ", "ī", "ı", "̍", "i", "̍",
    "i", "̋", "ó", "ò", "ô", "ǒ", "ō", "o", "̍", "ő", "ó",
    "͘", "ò", "͘", "ô", "͘", "ǒ", "͘", "ō", "͘", "o",
    "̍", "͘", "ő", "͘", "ú", "ù", "û", "ǔ", "ū", "u",
    "̍", "ű", "ḿ", "m", "̀", "m", "̂", "m", "̌", "m",
    "̄", "m", "̍", "m", "̋", "ń", "ǹ", "n", "̂", "ň", "n",
    "̄", "n", "̍", "n", "̋", "ⁿ"};
  int len = sizeof(pi)/sizeof(pi[0]);
  for(size_t i=0; i< len; i++) {
    if (boost::algorithm::ends_with(text, pi[i])) {
      return true;
    }
  }
  // p, t, k, h, r, g
  return isalpha(text.back());
}


bool ThauKamLomaji(const string& text) {
  const string pi[] = {
    "a", "e", "i", "o", "u",
    "A", "E", "I", "O", "U",
    "á", "à", "â", "ǎ", "ā", "a", "̍", "a",
    "̋", "é", "è", "ê", "ě", "ē", "e", "̍", "e",
    "̋", "í", "ì", "î", "ǐ", "ī", "ı", "̍", "i", "̍",
    "i", "̋", "ó", "ò", "ô", "ǒ", "ō", "o", "̍", "ő", "ó",
    "͘", "ò", "͘", "ô", "͘", "ǒ", "͘", "ō", "͘", "o",
    "̍", "͘", "ő", "͘", "ú", "ù", "û", "ǔ", "ū", "u",
    "̍", "ű", "ḿ", "m", "̀", "m", "̂", "m", "̌", "m",
    "̄", "m", "̍", "m", "̋", "ń", "ǹ", "n", "̂", "ň", "n",
    "̄", "n", "̍", "n", "̋", "ⁿ"};
  int len = sizeof(pi)/sizeof(pi[0]);
  for(size_t i=0; i< len; i++) {
    if (boost::algorithm::starts_with(text, pi[i])) {
      return true;
    }
  }
  // Siann-bó
  return isalpha(text.front());
}

// Sentence
bool KamAiLianJiHu(const string& ting_text,
                    const string& tsit_text) {
  if (ting_text.empty() || ting_text == " ") {
    return false;
  }

  bool phuann;
  bool ting_kam_lomaji = BueKamLomaji(ting_text);
  bool tsit_kam_lomaji = ThauKamLomaji(tsit_text);

  phuann = ting_kam_lomaji && tsit_kam_lomaji;

  LOG(ERROR) << "=====";
  LOG(WARNING) << ting_text << " = bue";
  LOG(WARNING) << ting_kam_lomaji << " = kam_ting_lomaji";
  LOG(WARNING) << tsit_text << " = thau";
  LOG(WARNING) << tsit_kam_lomaji << " = tsit_kam_lomaji";
  return phuann;
}

void Sentence::Extend(const DictEntry& entry,
                      size_t end_pos,
                      bool is_rear,
                      const string& preceding_text,
                      Grammar* grammar) {
  const string& context = empty() ? preceding_text : text();
  entry_->weight += Grammar::Evaluate(context, entry, is_rear, grammar);
  if(KamAiLianJiHu(entry_->text, entry.text)) {
    entry_->text.append("-");
  }
  entry_->text.append(entry.text);
  entry_->code.insert(entry_->code.end(),
                      entry.code.begin(),
                      entry.code.end());
  components_.push_back(entry);
  syllable_lengths_.push_back(end_pos - end());
  set_end(end_pos);
  DLOG(INFO) << "extend sentence " << end_pos << ") "
             << text() << " weight: " << weight();
}

void Sentence::Offset(size_t offset) {
  set_start(start() + offset);
  set_end(end() + offset);
}

// TranslatorOptions

TranslatorOptions::TranslatorOptions(const Ticket& ticket) {
  if (!ticket.schema)
    return;
  if (Config *config = ticket.schema->config()) {
    config->GetString(ticket.name_space + "/delimiter", &delimiters_) ||
        config->GetString("speller/delimiter", &delimiters_);
    config->GetString(ticket.name_space + "/tag", &tag_);
    config->GetBool(ticket.name_space + "/contextual_suggestions",
                    &contextual_suggestions_);
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

bool TranslatorOptions::IsUserDictDisabledFor(const string& input) const {
  if (user_dict_disabling_patterns_.empty())
    return false;
  for (const auto& pattern : user_dict_disabling_patterns_) {
    if (boost::regex_match(input, pattern))
      return true;
  }
  return false;
}

}  // namespace rime
