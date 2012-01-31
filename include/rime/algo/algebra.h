// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-01-19 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_ALGEBRA_H_
#define RIME_ALGEBRA_H_

#include <map>
#include <string>
#include <vector>
#include <rime/common.h>
#include <rime/config.h>
#include "spelling.h"

namespace rime {

class Calculation;
class Schema;

class Script : public std::map<std::string, std::vector<Spelling> > {
 public:
  bool AddSyllable(const std::string& syllable);
  void Merge(const std::string& s,
             const SpellingProperties& sp,
             const std::vector<Spelling>& v);
};

class Projection {
 public:
  bool Load(ConfigListPtr settings);
  // "spelling" -> "gnilleps"
  bool Apply(std::string* value);
  // {z, y, x} -> {a, b, c, d}
  bool Apply(Script* value);
 protected:
  std::vector<shared_ptr<Calculation> > calculation_;
};

}  // namespace rime

#endif  // RIME_ALGEBRA_H_
