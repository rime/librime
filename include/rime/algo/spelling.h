//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_SPELLING_H_
#define RIME_SPELLING_H_

#include <string>

namespace rime {

enum SpellingType { kNormalSpelling, kFuzzySpelling,
                    kAbbreviation, kCompletion, kAmbiguousSpelling,
                    kInvalidSpelling };

struct SpellingProperties {
  SpellingType type = kNormalSpelling;
  size_t end_pos = 0;
  double credibility = 1.0;
  std::string tips;
};

struct Spelling {
  std::string str;
  SpellingProperties properties;

  Spelling() = default;
  Spelling(const std::string& _str) : str(_str) {}

  bool operator== (const Spelling& other) { return str == other.str; }
  bool operator< (const Spelling& other) { return str < other.str; }
};

}  // namespace rime

#endif  // RIME_SPELLING_H_
