//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-07-17 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <utf8.h>
#include <rime/config.h>
#include <rime/algo/encoder.h>

namespace rime {

static const int kEncoderDfsLimit = 32;
static const int kMaxPhraseLength = 32;

string RawCode::ToString() const {
  return boost::join(*this, " ");
}

void RawCode::FromString(const string &code_str) {
  boost::split(*dynamic_cast<vector<string> *>(this),
               code_str,
               boost::algorithm::is_space(),
               boost::algorithm::token_compress_on);
}

TableEncoder::TableEncoder(PhraseCollector* collector)
    : Encoder(collector), loaded_(false), max_phrase_length_(0) {
}

/*
  # sample encoder configuration (from cangjie5.dict.yaml)
  encoder:
  exclude_patterns:
  - '^x.*$'
  - '^z.*$'
  rules:
  - length_equal: 2
  formula: "AaAzBaBbBz"
  - length_equal: 3
  formula: "AaAzBaBzCz"
  - length_in_range: [4, 10]
  formula: "AaBzCaYzZz"
  tail_anchor: "'"
*/
bool TableEncoder::LoadSettings(Config* config) {
  loaded_ = false;
  max_phrase_length_ = 0;
  encoding_rules_.clear();
  exclude_patterns_.clear();
  tail_anchor_.clear();

  if (!config) return false;

  if (auto rules = config->GetList("encoder/rules")) {
    for (auto it = rules->begin(); it != rules->end(); ++it) {
      auto rule = As<ConfigMap>(*it);
      if (!rule || !rule->HasKey("formula"))
        continue;
      const string formula(rule->GetValue("formula")->str());
      TableEncodingRule r;
      if (!ParseFormula(formula, &r))
        continue;
      r.min_word_length = r.max_word_length = 0;
      if (an<ConfigValue> value = rule->GetValue("length_equal")) {
        int length = 0;
        if (!value->GetInt(&length)) {
          LOG(ERROR) << "invalid length";
          continue;
        }
        r.min_word_length = r.max_word_length = length;
        if (max_phrase_length_ < length) {
          max_phrase_length_ = length;
        }
      }
      else if (auto range = As<ConfigList>(rule->Get("length_in_range"))) {
        if (range->size() != 2 ||
            !range->GetValueAt(0) ||
            !range->GetValueAt(1) ||
            !range->GetValueAt(0)->GetInt(&r.min_word_length) ||
            !range->GetValueAt(1)->GetInt(&r.max_word_length) ||
            r.min_word_length > r.max_word_length) {
          LOG(ERROR) << "invalid range.";
          continue;
        }
        if (max_phrase_length_ < r.max_word_length) {
          max_phrase_length_ = r.max_word_length;
        }
      }
      encoding_rules_.push_back(r);
    }
    if (max_phrase_length_ > kMaxPhraseLength) {
      max_phrase_length_ = kMaxPhraseLength;
    }
  }
  if (auto excludes = config->GetList("encoder/exclude_patterns")) {
    for (auto it = excludes->begin(); it != excludes->end(); ++it) {
      auto pattern = As<ConfigValue>(*it);
      if (!pattern)
        continue;
      exclude_patterns_.push_back(boost::regex(pattern->str()));
    }
  }
  config->GetString("encoder/tail_anchor", &tail_anchor_);

  loaded_ = !encoding_rules_.empty();
  return loaded_;
}

bool TableEncoder::ParseFormula(const string& formula,
                                TableEncodingRule* rule) {
  if (formula.length() % 2 != 0) {
    LOG(ERROR) << "bad formula: '%s'" << formula;
    return false;
  }
  for (auto it = formula.cbegin(), end = formula.cend(); it != end; ) {
    CodeCoords c;
    if (*it < 'A' || *it > 'Z') {
      LOG(ERROR) << "invalid character index in formula: '%s'" << formula;
      return false;
    }
    c.char_index = (*it >= 'U') ? (*it - 'Z' - 1) : (*it - 'A');
    ++it;
    if (*it < 'a' || *it > 'z') {
      LOG(ERROR) << "invalid code index in formula: '%s'" << formula;
      return false;
    }
    c.code_index = (*it >= 'u') ? (*it - 'z' - 1) : (*it - 'a');
    ++it;
    rule->coords.push_back(c);
  }
  return true;
}

bool TableEncoder::IsCodeExcluded(const string& code) {
  for (const boost::regex& pattern : exclude_patterns_) {
    if (boost::regex_match(code, pattern))
      return true;
  }
  return false;
}

bool TableEncoder::Encode(const RawCode& code, string* result) {
  int num_syllables = static_cast<int>(code.size());
  for (const TableEncodingRule& rule : encoding_rules_) {
    if (num_syllables < rule.min_word_length ||
        num_syllables > rule.max_word_length) {
      continue;
    }
    result->clear();
    CodeCoords previous = {0, 0};
    CodeCoords encoded = {0, 0};
    for (const CodeCoords& current : rule.coords) {
      CodeCoords c(current);
      if (c.char_index < 0) {
        c.char_index += num_syllables;
      }
      if (c.char_index >= num_syllables) {
        continue;  // 'abc def' ~ 'Ca'
      }
      if (c.char_index < 0) {
        continue;  // 'abc def' ~ 'Xa'
      }
      if (current.char_index < 0 &&
          c.char_index < encoded.char_index) {
        continue;  // 'abc def' ~ '(AaBa)Ya'
        // 'abc def' ~ '(AaBa)Aa' is OK
      }
      int start_index = 0;
      if (c.char_index == encoded.char_index) {
        start_index = encoded.code_index + 1;
      }
      c.code_index = CalculateCodeIndex(code[c.char_index],  c.code_index,
                                        start_index);
      if (c.code_index >= static_cast<int>(code[c.char_index].length())) {
        continue;  // 'abc def' ~ 'Ad'
      }
      if (c.code_index < 0) {
        continue;  // 'abc def' ~ 'Ax'
      }
      if ((current.char_index < 0 || current.code_index < 0) &&
          c.char_index == encoded.char_index &&
          c.code_index <= encoded.code_index &&
          (current.char_index != previous.char_index ||
           current.code_index != previous.code_index)) {
        continue;  // 'abc def' ~ '(AaBb)By', '(AaBb)Zb', '(AaZb)Zy'
        // 'abc def' ~ '(AaZb)Zb' is OK
        // 'abc def' ~ '(AaZb)Zz' is OK
      }
      *result += code[c.char_index][c.code_index];
      previous = current;
      encoded = c;
    }
    if (result->empty()) {
      continue;
    }
    return true;
  }

  return false;
}

// index: 0-based virtual index of encoding characters in `code`.
//        counting from the end of `code` if `index` is negative.
//        tail anchors do not count as encoding characters.
// start: when `index` is negative, the first appearance of a tail anchor
//        beyond `start` is used to locate the encoding character at index -1.
// returns string index in `code` for the character at virtual `index`.
// may return a negative number if `index` does not exist in `code`.
int TableEncoder::CalculateCodeIndex(const string& code, int index,
                                     int start) {
  DLOG(INFO) << "code = " << code
             << ", index = " << index << ", start = " << start;
  // tail_anchor = '|'
  const int n = static_cast<int>(code.length());
  int k = 0;
  if (index < 0) {
    // 'ab|cd|ef|g' ~ '(Aa)Az' -> 'ab'; start = 1, index = -1
    // 'ab|cd|ef|g' ~ '(AaAb)Az' -> 'abd'; start = 4, index = -1
    // 'ab|cd|ef|g' ~ '(AaAb)Ay' -> 'abc'; start = 4, index = -2
    k = n - 1;
    size_t tail = code.find_first_of(tail_anchor_, start + 1);
    if (tail != string::npos) {
      k = static_cast<int>(tail) - 1;
    }
    while (++index < 0) {
      while (--k >= 0 &&
             tail_anchor_.find(code[k]) != string::npos) {
      }
    }
  }
  else {
    // 'ab|cd|ef|g' ~ '(AaAb)Ac' -> 'abc'; index = 2
    while (index-- > 0) {
      while (++k < n &&
             tail_anchor_.find(code[k]) != string::npos) {
      }
    }
  }
  return k;
}

bool TableEncoder::EncodePhrase(const string& phrase,
                                const string& value) {
  size_t phrase_length = utf8::unchecked::distance(
      phrase.c_str(), phrase.c_str() + phrase.length());
  if (static_cast<int>(phrase_length) > max_phrase_length_)
    return false;

  RawCode code;
  int limit = kEncoderDfsLimit;
  return DfsEncode(phrase, value, 0, &code, &limit);
}

bool TableEncoder::DfsEncode(const string& phrase,
                             const string& value,
                             size_t start_pos,
                             RawCode* code,
                             int* limit) {
  if (start_pos == phrase.length()) {
    if (limit) {
      --*limit;
    }
    string encoded;
    if (Encode(*code, &encoded)) {
      DLOG(INFO) << "encode '" << phrase << "': "
                 << "[" << code->ToString() << "] -> [" << encoded << "]";
      collector_->CreateEntry(phrase, encoded, value);
      return true;
    }
    else {
      DLOG(WARNING) << "failed to encode '" << phrase << "': "
                    << "[" << code->ToString() << "]";
      return false;
    }
  }
  const char* word_start = phrase.c_str() + start_pos;
  const char* word_end = word_start;
  utf8::unchecked::next(word_end);
  size_t word_len = word_end - word_start;
  string word(word_start, word_len);
  bool ret = false;
  vector<string> translations;
  if (collector_->TranslateWord(word, &translations)) {
    for (const string& x : translations) {
      if (IsCodeExcluded(x)) {
        continue;
      }
      code->push_back(x);
      bool ok = DfsEncode(phrase, value, start_pos + word_len, code, limit);
      ret = ret || ok;
      code->pop_back();
      if (limit && *limit <= 0) {
        return ret;
      }
    }
  }
  return ret;
}

ScriptEncoder::ScriptEncoder(PhraseCollector* collector)
    : Encoder(collector) {
}

bool ScriptEncoder::EncodePhrase(const string& phrase,
                                 const string& value) {
  size_t phrase_length = utf8::unchecked::distance(
      phrase.c_str(), phrase.c_str() + phrase.length());
  if (static_cast<int>(phrase_length) > kMaxPhraseLength)
    return false;

  RawCode code;
  int limit = kEncoderDfsLimit;
  return DfsEncode(phrase, value, 0, &code, &limit);
}

bool ScriptEncoder::DfsEncode(const string& phrase,
                              const string& value,
                              size_t start_pos,
                              RawCode* code,
                              int* limit) {
  if (start_pos == phrase.length()) {
    if (limit) {
      --*limit;
    }
    collector_->CreateEntry(phrase, code->ToString(), value);
    return true;
  }
  bool ret = false;
  for (size_t k = phrase.length() - start_pos; k > 0; --k) {
    string word(phrase.substr(start_pos, k));
    vector<string> translations;
    if (collector_->TranslateWord(word, &translations)) {
      for (const string& x : translations) {
        code->push_back(x);
        bool ok = DfsEncode(phrase, value, start_pos + k, code, limit);
        ret = ret || ok;
        code->pop_back();
        if (limit && *limit <= 0) {
          return ret;
        }
      }
    }
  }
  return ret;
}

}  // namespace rime
