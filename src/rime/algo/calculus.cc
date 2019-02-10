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

const double kAbbreviationPenalty = -0.6931471805599453; // log(0.5)
const double kFuzzySpellingPenalty = -0.6931471805599453; // log(0.5)

Calculus::Calculus() {
  Register("xlit", &Transliteration::Parse);
  Register("xform", &Transformation::Parse);
  Register("erase", &Erasion::Parse);
  Register("derive", &Derivation::Parse);
  Register("fuzz", &Fuzzing::Parse);
  Register("abbrev", &Abbreviation::Parse);
}

void Calculus::Register(const string& token,
                        Calculation::Factory* factory) {
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
  while ((cl = utf8::unchecked::next(pl)),
         (cr = utf8::unchecked::next(pr)),
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
  bool modified = false;
  const char* p = spelling->str.c_str();
  const int buffer_len = 256;
  char buffer[buffer_len] = "";
  char* q = buffer;
  uint32_t c;
  while ((c = utf8::unchecked::next(p))) {
    if (q - buffer > buffer_len - 7) {  // insufficient space
      modified = false;
      break;
    }
    if (char_map_.find(c) != char_map_.end()) {
      c = char_map_[c];
      modified = true;
    }
    q = utf8::unchecked::append(c, q);
  }
  if (modified) {
    *q = '\0';
    spelling->str.assign(buffer);
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
  string result = boost::regex_replace(spelling->str,
                                       pattern_, replacement_);
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

}  // namespace rime
