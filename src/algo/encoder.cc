//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-07-17 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/config.h>
#include <rime/algo/encoder.h>

namespace rime {

TableEncoder::TableEncoder() : loaded_(false) {
}

void TableEncoder::LoadSettings(Config* config) {
  loaded_ = false;
  encoding_rules_.clear();
  exclude_patterns_.clear();
  tail_anchor_.clear();
  if (!config) return;
  if (ConfigListPtr rules = config->GetList("encoder/rules")) {
    for (ConfigList::Iterator it = rules->begin(); it != rules->end(); ++it) {
      ConfigMapPtr rule = As<ConfigMap>(*it);
      if (!rule || !rule->HasKey("formula"))
        continue;
      const std::string formula(rule->GetValue("formula")->str());
      TableEncodingRule r;
      if (!ParseFormula(formula, &r))
        continue;
      r.min_word_length = r.max_word_length = 0;
      if (ConfigValuePtr value = rule->GetValue("length_equal")) {
        int length = 0;
        if (!value->GetInt(&length)) {
          LOG(ERROR) << "invalid length";
          continue;
        }
        r.min_word_length = r.max_word_length = length;
      }
      else if (ConfigListPtr range =
               As<ConfigList>(rule->Get("length_in_range"))) {
        if (range->size() != 2 ||
            !range->GetValueAt(0) ||
            !range->GetValueAt(1) ||
            !range->GetValueAt(0)->GetInt(&r.min_word_length) ||
            !range->GetValueAt(1)->GetInt(&r.max_word_length) ||
            r.min_word_length > r.max_word_length) {
          LOG(ERROR) << "invalid range.";
          continue;
        }
      }
      encoding_rules_.push_back(r);
    }
  }
  loaded_ = !encoding_rules_.empty();
  if (ConfigListPtr excludes = config->GetList("encoder/exclude_patterns")) {
    for (ConfigList::Iterator it = excludes->begin();
         it != excludes->end(); ++it) {
      ConfigValuePtr pattern = As<ConfigValue>(*it);
      if (!pattern)
        continue;
      exclude_patterns_.push_back(boost::regex(pattern->str()));
    }
  }
  config->GetString("encoder/tail_anchor", &tail_anchor_);
}

bool TableEncoder::LoadFromFile(const std::string& filename) {
  Config config;
  if (!config.LoadFromFile(filename)) {
    return false;
  }
  LoadSettings(&config);
  return loaded();
}

bool TableEncoder::ParseFormula(const std::string& formula,
                                TableEncodingRule* rule) {
  if (formula.length() % 2 != 0) {
    LOG(ERROR) << "bad formula: '%s'" << formula;
    return false;
  }
  for (std::string::const_iterator it = formula.begin(), end = formula.end();
       it != end; ) {
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

bool TableEncoder::IsCodeExcluded(const std::string& code) {
  BOOST_FOREACH(const boost::regex& pattern, exclude_patterns_) {
    if (boost::regex_match(code, pattern))
      return true;
  }
  return false;
}

bool TableEncoder::Encode(const std::vector<std::string>& code,
                          std::string* result) {
  int num_syllables = static_cast<int>(code.size());
  BOOST_FOREACH(const TableEncodingRule& rule, encoding_rules_) {
    if (num_syllables < rule.min_word_length ||
        num_syllables > rule.max_word_length) {
      continue;
    }
    result->clear();
    CodeCoords previous = {0, 0};
    CodeCoords encoded = {0, 0};
    BOOST_FOREACH(const CodeCoords& current, rule.coords) {
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

int TableEncoder::CalculateCodeIndex(const std::string& code, int index,
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
    if (tail != std::string::npos) {
      k = static_cast<int>(tail) - 1;
    }
    while (++index < 0) {
      while (--k >= 0 &&
             tail_anchor_.find(code[k]) != std::string::npos) {
      }
    }
  }
  else {
    // 'ab|cd|ef|g' ~ '(AaAb)Ac' -> 'abc'; index = 2
    while (index-- > 0) {
      while (++k < n &&
             tail_anchor_.find(code[k]) != std::string::npos) {
      }
    }
  }
  return k;
}

}  // namespace rime
