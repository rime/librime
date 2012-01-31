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

bool Transliteration::Apply(const Spelling& input, Spelling* output) {
  if (input.str.empty() || !output)
    return false;
  bool modified = false;
  const char* p = input.str.c_str();
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
    output->str.assign(buffer);
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

bool Transformation::Apply(const Spelling& input, Spelling* output) {
  if (input.str.empty() || !output)
    return false;
  std::string result(boost::regex_replace(input.str, pattern_, replacement_));
  if (result == input.str)
    return false;
  output->str.swap(result);
  return true;
}

}  // namespace rime
