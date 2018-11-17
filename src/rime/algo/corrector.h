//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/14.
//

#ifndef RIME_CORRECTOR_H
#define RIME_CORRECTOR_H

#include <rime/common.h>
#include <rime/dict/vocabulary.h>
#include <rime/dict/prism.h>
#include "spelling.h"
#include "algebra.h"

namespace rime {

class CorrectionCollector {
 public:
  explicit CorrectionCollector(const Syllabary& syllabary): syllabary_(syllabary) {}

  Script Collect(size_t edit_distance);

 private:
  const Syllabary& syllabary_;
};

class Corrector : public Prism {
 public:
  using Distance = uint8_t;
  vector<Match> SymDeletePrefixSearch(const string& key);
  static Distance LevenshteinDistance(const std::string &s1, const std::string &s2);
  static Distance RestrictedDistance(const std::string& s1, const std::string& s2);
};

} // namespace rime

#endif //RIME_CORRECTOR_H
