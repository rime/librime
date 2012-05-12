// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_SPELLING_H_
#define RIME_SPELLING_H_

#include <string>

namespace rime {

enum SpellingType { kNormalSpelling, kAbbreviation, kCompletion,
                    kAmbiguousSpelling, kInvalidSpelling };

struct SpellingProperties {
  SpellingType type;
  size_t end_pos;
  double credibility;
  std::string tips;
  SpellingProperties() : type(kNormalSpelling), end_pos(0), credibility(1.0) {}
};

struct Spelling {
  std::string str;
  SpellingProperties properties;
  Spelling() {}
  Spelling(const std::string& _str) : str(_str), properties() {}
  bool operator== (const Spelling& other) { return str == other.str; }
  bool operator< (const Spelling& other) { return str < other.str; }
};

}  // namespace rime

#endif  // RIME_SPELLING_H_
