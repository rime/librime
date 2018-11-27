//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_SPELLING_H_
#define RIME_SPELLING_H_

#include <rime/common.h>

namespace rime {

enum SpellingType { kNormalSpelling, kFuzzySpelling,
                    kAbbreviation, kCompletion, kAmbiguousSpelling,
                    kInvalidSpelling };

struct SpellingProperties {
  SpellingType type = kNormalSpelling;
  size_t end_pos = 0;
  double credibility = 1.0;
  string tips;
};

struct Spelling {
  string str;
  SpellingProperties properties;

  Spelling() = default;
  Spelling(const string& _str) : str(_str) {}

  bool operator== (const Spelling& other) { return str == other.str; }
  bool operator< (const Spelling& other) { return str < other.str; }
};

}  // namespace rime

#endif  // RIME_SPELLING_H_
