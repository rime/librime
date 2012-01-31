// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <utf8.h>
#include <rime/algo/calculus.h>

namespace rime {

Calculus::Calculus() {
  Register("xlit", &Transliteration::Parse);
  Register("xform", &Transformation::Parse);
  Register("erase", &Erasion::Parse);
  Register("derive", &Derivation::Parse);
  Register("abbrev", &Abbreviation::Parse);
}

void Calculus::Register(const std::string& token,
                        Calculation::Factory* factory) {
  factories_[token] = factory;
}

Calculation* Calculus::Parse(const std::string& definition) {
  size_t sep = definition.find_first_not_of("zyxwvutsrqponmlkjihgfedcba");
  if (sep == std::string::npos)
    return NULL;
  std::vector<std::string> args;
  boost::split(args, definition,
               boost::is_from_range(definition[sep], definition[sep]));
  if (args.empty())
    return NULL;
  std::map<std::string,
           Calculation::Factory*>::iterator it = factories_.find(args[0]);
  if (it == factories_.end())
    return NULL;
  Calculation* result = (*it->second)(args);
  return result;
}

// Transliteration

Calculation* Transliteration::Parse(const std::vector<std::string>& args) {
  if (args.size() < 3)
    return NULL;
  const std::string& left(args[1]);
  const std::string& right(args[2]);
  const char* pl = left.c_str();
  const char* pr = right.c_str();
  uint32_t cl, cr;
  std::map<uint32_t, uint32_t> char_map;
  while ((cl = utf8::unchecked::next(pl)),
         (cr = utf8::unchecked::next(pr)),
         cl && cr) {
    char_map[cl] = cr;
  }
  if (cl == 0 && cr == 0) {
    Transliteration* x = new Transliteration;
    x->char_map_.swap(char_map);
    return x;
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

Calculation* Transformation::Parse(const std::vector<std::string>& args) {
  if (args.size() < 3)
    return NULL;
  const std::string& left(args[1]);
  const std::string& right(args[2]);
  if (left.empty() || right.empty())
    return NULL;
  Transformation* x = new Transformation;
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x;
}

bool Transformation::Apply(Spelling* spelling) {
  if (!spelling || spelling->str.empty())
    return false;
  std::string result(boost::regex_replace(spelling->str,
                                          pattern_, replacement_));
  if (result == spelling->str)
    return false;
  spelling->str.swap(result);
  return true;
}

// Erasion

Calculation* Erasion::Parse(const std::vector<std::string>& args) {
  if (args.size() < 2)
    return NULL;
  const std::string& pattern(args[1]);
  if (pattern.empty())
    return NULL;
  Erasion* x = new Erasion;
  x->pattern_.assign(pattern);
  return x;
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

Calculation* Derivation::Parse(const std::vector<std::string>& args) {
  if (args.size() < 3)
    return NULL;
  const std::string& left(args[1]);
  const std::string& right(args[2]);
  if (left.empty() || right.empty())
    return NULL;
  Derivation* x = new Derivation;
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x;
}

// Abbreviation

Calculation* Abbreviation::Parse(const std::vector<std::string>& args) {
  if (args.size() < 3)
    return NULL;
  const std::string& left(args[1]);
  const std::string& right(args[2]);
  if (left.empty() || right.empty())
    return NULL;
  Abbreviation* x = new Abbreviation;
  x->pattern_.assign(left);
  x->replacement_.assign(right);
  return x;
}

bool Abbreviation::Apply(Spelling* spelling) {
  bool result = Transformation::Apply(spelling);
  if (result) {
    spelling->properties.type = kAbbreviation;
    spelling->properties.credibility *= 0.5;
  }
  return result;
}

}  // namespace rime
