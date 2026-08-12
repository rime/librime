//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <utf8.h>
#include <rime/algo/calculus.h>
#include <rime/common.h>

namespace rime {

const double kAbbreviationPenalty = -0.6931471805599453;   // log(0.5)
const double kFuzzySpellingPenalty = -0.6931471805599453;  // log(0.5)
const double kCorrectionPenalty = -4.605170185988091;      // log(0.01)

Calculus::Calculus() {
  Register("xlit", &Transliteration::Parse);
  Register("xform", &Transformation::Parse);
  Register("erase", &Erasion::Parse);
  Register("derive", &Derivation::Parse);
  Register("fuzz", &Fuzzing::Parse);
  Register("abbrev", &Abbreviation::Parse);
  Register("reorder", &Reorder::Parse);
}

void Calculus::Register(const string& token, Calculation::Factory* factory) {
  factories_[token] = factory;
}

Calculation* Calculus::Parse(const string& definition) {
  size_t sep = definition.find_first_not_of("zyxwvutsrqponmlkjihgfedcba");
  if (sep == string::npos)
    return NULL;
  vector<string> args;
  boost::split(args, definition,
               boost::is_from_range(definition[sep], definition[sep]));
  if (args.empty())
    return NULL;
  auto it = factories_.find(args[0]);
  if (it == factories_.end())
    return NULL;
  Calculation* result = (*it->second)(args);
  return result;
}

// Transliteration

Calculation* Transliteration::Parse(const vector<string>& args) {
  if (args.size() < 3)
    return NULL;
  const string& left(args[1]);
  const string& right(args[2]);
  const char* pl = left.c_str();
  const char* pr = right.c_str();
  uint32_t cl, cr;
  map<uint32_t, uint32_t> char_map;
  while ((cl = utf8::unchecked::next(pl)), (cr = utf8::unchecked::next(pr)),
         cl && cr) {
    char_map[cl] = cr;
  }
  if (cl == 0 && cr == 0) {
    the<Transliteration> x(new Transliteration);
    x->char_map_.swap(char_map);
    return x.release();
  }
  return NULL;
}

bool Transliteration::Apply(Spelling* spelling) {
  if (!spelling || spelling->str.empty())
    return false;

  const char* const start = spelling->str.c_str();
  const char* const end = start + spelling->str.size();

  string result;
  bool modified = false;

  const char* p = start;
  while (p < end) {
    const char* here = p;
    uint32_t c = utf8::unchecked::next(p);

    auto it = char_map_.find(c);
    if (it != char_map_.end()) {
      if (!modified) {
        // allocate space and copy unmodified prefix up to this point
        modified = true;
        result.reserve(spelling->str.size());
        result.assign(start, here - start);
      }
      c = it->second;  // replace character
    }

    if (modified) {
      utf8::unchecked::append(c, std::back_inserter(result));
    }
  }

  if (modified) {
    spelling->str = std::move(result);
  }

  return modified;
}

// Transformation

Calculation* Transformation::Parse(const vector<string>& args) {
  if (args.size() < 3)
    return NULL;
  const string& left(args[1]);
  const string& right(args[2]);
  if (left.empty())
    return NULL;
  the<Transformation> x(new Transformation);
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x.release();
}

bool Transformation::Apply(Spelling* spelling) {
  if (!spelling || spelling->str.empty())
    return false;
  string result = boost::regex_replace(spelling->str, pattern_, replacement_);
  if (result == spelling->str)
    return false;
  spelling->str.swap(result);
  return true;
}

// Erasion

Calculation* Erasion::Parse(const vector<string>& args) {
  if (args.size() < 2)
    return NULL;
  const string& pattern(args[1]);
  if (pattern.empty())
    return NULL;
  the<Erasion> x(new Erasion);
  x->pattern_.assign(pattern);
  return x.release();
}

bool Erasion::Apply(Spelling* spelling) {
  if (!spelling || spelling->str.empty())
    return false;
  if (!boost::regex_match(spelling->str, pattern_))
    return false;
  spelling->str.clear();
  return true;
}

// Derivation

Calculation* Derivation::Parse(const vector<string>& args) {
  if (args.size() < 3)
    return NULL;

  const string& left(args[1]);
  const string& right(args[2]);
  if (left.empty())
    return NULL;

  if (args.size() > 3) {
    const string& tag = args[3];
    // 糾錯
    if (tag == "correction") {
      the<Correction> x(new Correction);
      x->pattern_.assign(left);
      x->replacement_.assign(right);
      return x.release();
    }
    // 簡拼
    if (tag == "abbrev") {
      the<Abbreviation> x(new Abbreviation);
      x->pattern_.assign(left);
      x->replacement_.assign(right);
      return x.release();
    }
    // 模糊音
    if (tag == "fuzz") {
      the<Fuzzing> x(new Fuzzing);
      x->pattern_.assign(left);
      x->replacement_.assign(right);
      return x.release();
    }
    // tag 無法識別, 作爲普通 derive 處理
  }

  the<Derivation> x(new Derivation);
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x.release();
}

// Fuzzing

Calculation* Fuzzing::Parse(const vector<string>& args) {
  if (args.size() < 3)
    return NULL;
  const string& left(args[1]);
  const string& right(args[2]);
  if (left.empty())
    return NULL;
  the<Fuzzing> x(new Fuzzing);
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x.release();
}

bool Fuzzing::Apply(Spelling* spelling) {
  bool result = Transformation::Apply(spelling);
  if (result) {
    spelling->properties.type = kFuzzySpelling;
    spelling->properties.credibility += kFuzzySpellingPenalty;
  }
  return result;
}

// Abbreviation

Calculation* Abbreviation::Parse(const vector<string>& args) {
  if (args.size() < 3)
    return NULL;
  const string& left(args[1]);
  const string& right(args[2]);
  if (left.empty())
    return NULL;
  the<Abbreviation> x(new Abbreviation);
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x.release();
}

bool Abbreviation::Apply(Spelling* spelling) {
  bool result = Transformation::Apply(spelling);
  if (result) {
    spelling->properties.type = kAbbreviation;
    spelling->properties.credibility += kAbbreviationPenalty;
  }
  return result;
}

// Correction

bool Correction::Apply(Spelling* spelling) {
  bool result = Transformation::Apply(spelling);
  if (result) {
    spelling->properties.is_correction = true;
    spelling->properties.credibility += kCorrectionPenalty;
  }
  return result;
}

// Reorder

Calculation* Reorder::Parse(const vector<string>& args) {
  if (args.size() < 2)
    return nullptr;
  const string& order = args[1];
  if (order.empty())
    return nullptr;
  bool dedup = args.size() > 2 && args[2] == "dedup";
  the<Reorder> x(new Reorder);
  utf8::unchecked::utf8to32(order.begin(), order.end(),
                            std::back_inserter(x->order_));
  x->dedup_ = dedup;
  return x.release();
}

bool Reorder::Apply(Spelling* spelling) {
  if (!spelling || spelling->str.empty())
    return false;

  const char* const start = spelling->str.c_str();
  const char* const end = start + spelling->str.length();

  string result;
  bool modified = false;

  vector<int> key_count(order_.size(), 0);
  const char* group_start = nullptr;
  size_t last_key_index = 0;
  bool needs_reorder = false;

  // flush the reordered characters from the key group
  auto flush_group = [&](auto& output) {
    for (size_t k = 0; k < order_.size(); ++k) {
      if (key_count[k]) {
        const uint32_t key = order_.at(k);
        const size_t n = dedup_ ? 1 : key_count[k];
        for (size_t i = 0; i < n; ++i) {
          utf8::unchecked::append(key, output);
        }
      }
    }
  };

  // clear the key count of the current group
  auto clear_group = [&]() {
    std::fill(key_count.begin(), key_count.end(), 0);
    group_start = nullptr;
  };

  auto settle_group = [&](const char* group_end) {
    if (!group_start)
      return;

    if (needs_reorder) {
      if (!modified) {
        modified = true;
        // allocate space and copy unmodified prefix before the group
        result.reserve(spelling->str.size());
        result.assign(start, group_start - start);
      }
      auto output = std::back_inserter(result);
      flush_group(output);
    } else if (modified) {
      // copy the unmodified group to the result
      result.append(group_start, group_end - group_start);
    }
    clear_group();
  };

  const char* p = start;
  while (p < end) {
    const char* const here = p;
    uint32_t c = utf8::unchecked::next(p);

    size_t key_index = order_.find(c);
    if (key_index != std::u32string::npos) {
      // belongs to the key group to be reordered
      if (!group_start) {
        group_start = here;
        needs_reorder = false;
      } else if (key_index < last_key_index) {
        // the key is not in monotonically increasing order
        needs_reorder = true;
      }
      last_key_index = key_index;

      ++key_count[key_index];
      if (dedup_ && key_count[key_index] > 1) {
        needs_reorder = true;
      }
      continue;
    }

    // found a delimiting character; settle the previous key group
    settle_group(here);

    // append the current delimiting character
    if (modified) {
      auto output = std::back_inserter(result);
      utf8::unchecked::append(c, output);
    }
  }  // end of string traversal

  // settle the key group at the end of string
  settle_group(end);

  if (modified) {
    spelling->str = std::move(result);
  }

  return modified;
}

}  // namespace rime
