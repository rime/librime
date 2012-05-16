// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-16 Zou Xu <zouivex@gmail.com>
// 2012-01-26 GONG Chen <chen.sst@gmail.com>  spelling algebra support
//

#ifndef RIME_PRISM_H_
#define RIME_PRISM_H_

#include <set>
#include <string>
#include <vector>
#include <darts.h>
#include <rime/common.h>
#include <rime/algo/spelling.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/vocabulary.h>

namespace rime {

namespace prism {

typedef int32_t SyllableId;

struct SpellingDescriptor {
  SyllableId syllable_id;
  int32_t type;
  double credibility;
  String tips;
};

typedef List<SpellingDescriptor> SpellingMapItem;
typedef Array<SpellingMapItem> SpellingMap;

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t schema_file_checksum;
  uint32_t num_syllables;
  uint32_t num_spellings;
  uint32_t double_array_size;
  OffsetPtr<char> double_array;
  OffsetPtr<SpellingMap> spelling_map;
  char alphabet[256];
};

}  // namespace prism

class SpellingAccessor {
 public:
  SpellingAccessor(prism::SpellingMap* spelling_map, int spelling_id);
  bool Next();
  bool exhausted() const;
  int syllable_id() const;
  const SpellingProperties properties() const;
  
 protected:
  int spelling_id_;
  prism::SpellingDescriptor* iter_;
  prism::SpellingDescriptor* end_;
};

class Script;

class Prism : public MappedFile {
 public:
  typedef Darts::DoubleArray::result_pair_type Match;

  Prism(const std::string &file_name)
      : MappedFile(file_name), trie_(new Darts::DoubleArray),
        metadata_(NULL), spelling_map_(NULL), format_(0.0) {}

  bool Load();
  bool Save();
  bool Build(const Syllabary &syllabary,
             const Script *script = NULL,
             uint32_t dict_file_checksum = 0,
             uint32_t schema_file_checksum = 0);
  
  bool HasKey(const std::string &key);
  bool GetValue(const std::string &key, int *value);
  void CommonPrefixSearch(const std::string &key, std::vector<Match> *result);
  void ExpandSearch(const std::string &key, std::vector<Match> *result, size_t limit);
  const SpellingAccessor QuerySpelling(int spelling_id);

  size_t array_size() const;

  uint32_t dict_file_checksum() const;
  uint32_t schema_file_checksum() const;

 private:
  scoped_ptr<Darts::DoubleArray> trie_;
  prism::Metadata* metadata_;
  prism::SpellingMap* spelling_map_;
  double format_;
};

}  // namespace rime

#endif  // RIME_PRISM_H_
