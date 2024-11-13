//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-19 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_ALGEBRA_H_
#define RIME_ALGEBRA_H_

#include <rime/common.h>
#include <rime/config.h>
#include "spelling.h"

namespace rime {

class Calculation;
class Schema;

class Script : public map<string, vector<Spelling>, std::less<>> {
 public:
  RIME_API bool AddSyllable(string_view syllable);
  void Merge(string_view s,
             const SpellingProperties& sp,
             const vector<Spelling>& v);
  void Dump(const path& file_path) const;
};

class Projection {
 public:
  RIME_API bool Load(an<ConfigList> settings);
  // "spelling" -> "gnilleps"
  RIME_API bool Apply(string* value);
  // {z, y, x} -> {a, b, c, d}
  RIME_API bool Apply(Script* value);

 protected:
  vector<of<Calculation>> calculation_;
};

}  // namespace rime

#endif  // RIME_ALGEBRA_H_
