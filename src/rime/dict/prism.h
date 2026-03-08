//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-16 Zou Xu <zouivex@gmail.com>
// 2012-01-26 GONG Chen <chen.sst@gmail.com>  spelling algebra support
//

#ifndef RIME_PRISM_H_
#define RIME_PRISM_H_

#include <darts.h>
#include <rime/common.h>
#include <rime/algo/spelling.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/vocabulary.h>

namespace rime {

namespace prism {

using Credibility = float;

struct SpellingDescriptor {
  SyllableId syllable_id;
  // bit 30: is_correction
  int32_t type;
  Credibility credibility;
  String tips;
};

using SpellingMapItem = List<SpellingDescriptor>;
using SpellingMap = Array<SpellingMapItem>;

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t schema_file_checksum;
  uint32_t num_syllables;
  uint32_t num_spellings;
  uint32_t double_array_size;
  OffsetPtr<char> double_array;
  // v1.0
  OffsetPtr<SpellingMap> spelling_map;
  char alphabet[256];
};

}  // namespace prism

class SpellingAccessor {
 public:
  SpellingAccessor(prism::SpellingMap* spelling_map, SyllableId spelling_id);
  bool Next();
  bool exhausted() const;
  SyllableId syllable_id() const;
  SpellingProperties properties() const;

 protected:
  SyllableId spelling_id_;
  prism::SpellingDescriptor* iter_;
  prism::SpellingDescriptor* end_;
};

class Script;

class Prism : public MappedFile {
 public:
  using Match = Darts::DoubleArray::result_pair_type;

  RIME_DLL explicit Prism(const path& file_path);

  RIME_DLL bool Load();
  RIME_DLL bool Save();
  RIME_DLL bool Build(const Syllabary& syllabary,
                      const Script* script = nullptr,
                      uint32_t dict_file_checksum = 0,
                      uint32_t schema_file_checksum = 0);

  RIME_DLL bool HasKey(const string& key);
  RIME_DLL bool GetValue(const string& key, int* value) const;
  RIME_DLL void CommonPrefixSearch(const string& key, vector<Match>* result);
  RIME_DLL void ExpandSearch(const string& key,
                             vector<Match>* result,
                             size_t limit);
  SpellingAccessor QuerySpelling(SyllableId spelling_id);

  RIME_DLL size_t array_size() const;

  uint32_t dict_file_checksum() const;
  uint32_t schema_file_checksum() const;
  Darts::DoubleArray& trie() const { return *trie_; }

 protected:
  the<Darts::DoubleArray> trie_;
  prism::Metadata* metadata_ = nullptr;
  prism::SpellingMap* spelling_map_ = nullptr;
  double format_ = 0.0;
};

}  // namespace rime

#endif  // RIME_PRISM_H_
