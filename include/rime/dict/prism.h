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

struct SpellingDescriptor {
  SyllableId syllable_id;
  int32_t type;
  double credibility;
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

  explicit Prism(const string& file_name);

  bool Load();
  bool Save();
  bool Build(const Syllabary& syllabary,
             const Script* script = NULL,
             uint32_t dict_file_checksum = 0,
             uint32_t schema_file_checksum = 0);

  bool HasKey(const string& key);
  bool GetValue(const string& key, int* value);
  void CommonPrefixSearch(const string& key, vector<Match>* result);
  void ExpandSearch(const string& key, vector<Match>* result, size_t limit);
  SpellingAccessor QuerySpelling(SyllableId spelling_id);

  size_t array_size() const;

  uint32_t dict_file_checksum() const;
  uint32_t schema_file_checksum() const;

 private:
  the<Darts::DoubleArray> trie_;
  prism::Metadata* metadata_ = nullptr;
  prism::SpellingMap* spelling_map_ = nullptr;
  double format_ = 0.0;
};

}  // namespace rime

#endif  // RIME_PRISM_H_
