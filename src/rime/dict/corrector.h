//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/14.
//

#ifndef RIME_CORRECTOR_H
#define RIME_CORRECTOR_H

#include <rime/common.h>
#include <rime/component.h>
#include <rime/algo/algebra.h>
#include <rime/algo/spelling.h>
#include <rime/dict/prism.h>
#include <rime/dict/vocabulary.h>

namespace rime {
struct Ticket;

class SymDeleteCollector {
 public:
  explicit SymDeleteCollector(const Syllabary& syllabary): syllabary_(syllabary) {}

  Script Collect(size_t edit_distance);

 private:
  const Syllabary& syllabary_;
};

namespace corrector {
using Distance = size_t;
struct Correction {
  size_t distance;
  SyllableId syllable;
  size_t length;
};
class Corrections : public hash_map<SyllableId, Correction> {
 public:
  /// Update for better correction
  /// \param syllable
  /// \param correction
  inline void Alter(SyllableId syllable, Correction correction) {
    if (find(syllable) == end() || correction.distance < (*this)[syllable].distance) {
      (*this)[syllable] = correction;
    }
  };
};
} // namespace corrector

/**
 * The unify interface of correctors
 */
class Corrector : public Class<Corrector, const Ticket&> {
 public:
  virtual ~Corrector() = default;
  RIME_API virtual void ToleranceSearch(const Prism &prism,
                                        const string &key,
                                        corrector::Corrections *results,
                                        size_t tolerance) = 0;
};

class CorrectorComponent : public Corrector::Component {
 public:
  CorrectorComponent();
  ~CorrectorComponent() override = default;
  Corrector *Create(const Ticket& ticket) noexcept override;
 private:
  template<class ...Cs>
  static Corrector *Combine(Cs ...args);

  map<string, weak<Corrector>> correctors_;
  the<ResourceResolver> resolver_;

  class Unified : public Corrector {
   public:
    Unified() = default;
    RIME_API void ToleranceSearch(const Prism &prism,
                                  const string &key,
                                  corrector::Corrections *results,
                                  size_t tolerance) override;
    template<class ...Cs>
    void Add(Cs ...args) {
      contents = { args... };
    }

   private:
    vector<of<Corrector>> contents = {};
  };
};


class EditDistanceCorrector : public Corrector,
                              public Prism {
 public:
  ~EditDistanceCorrector() override = default;
  RIME_API explicit EditDistanceCorrector(const string& file_name);

  RIME_API bool Build(const Syllabary& syllabary,
                      const Script* script = nullptr,
                      uint32_t dict_file_checksum = 0,
                      uint32_t schema_file_checksum = 0);

  RIME_API void ToleranceSearch(const Prism &prism,
                                const string &key,
                                corrector::Corrections *results,
                                size_t tolerance) override;
  corrector::Distance LevenshteinDistance(const std::string &s1, const std::string &s2);
  corrector::Distance RestrictedDistance(const std::string& s1, const std::string& s2, corrector::Distance threshold);
};

class NearSearchCorrector : public Corrector {
 public:
  NearSearchCorrector() = default;
  ~NearSearchCorrector() override = default;
  RIME_API void ToleranceSearch(const Prism &prism,
                                const string &key,
                                corrector::Corrections *results,
                                size_t tolerance) override;
};

template<class... Cs>
Corrector *CorrectorComponent::Combine(Cs ...args) {
  auto u = new Unified();
  u->Add(args...);
  return u;
}

} // namespace rime

#endif //RIME_CORRECTOR_H
