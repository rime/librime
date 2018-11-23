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

//class Corrector {
// public:
//  RIME_API virtual bool Build(const Syllabary& syllabary,
//                      const Script* script,
//                      uint32_t dict_file_checksum,
//                      uint32_t schema_file_checksum) = 0;
//};

class Corrector : public Prism {
 public:
  using Distance = uint8_t;

  RIME_API explicit Corrector(const string& file_name) : Prism(file_name) {}

  RIME_API bool Build(const Syllabary& syllabary,
                      const Script* script,
                      uint32_t dict_file_checksum,
                      uint32_t schema_file_checksum) override;

  vector<Match> SymDeletePrefixSearch(const string& key);
  static Distance LevenshteinDistance(const std::string &s1, const std::string &s2);
  static Distance RestrictedDistance(const std::string& s1, const std::string& s2);
};

class NearSearchCorrector {
 public:
  using Correction = struct {
    size_t distance;
    SyllableId syllable;
    size_t length;
  };
  NearSearchCorrector() = default;
  RIME_API vector<Correction> ToleranceSearch(const Prism& prism, const string& key, size_t tolerance = 5);
};

} // namespace rime

#endif //RIME_CORRECTOR_H
