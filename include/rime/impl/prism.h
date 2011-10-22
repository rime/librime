// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-5-16 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_PRISM_H_
#define RIME_PRISM_H_

#include <set>
#include <string>
#include <vector>
#include <darts.h>
#include <rime/common.h>
#include <rime/impl/mapped_file.h>
#include <rime/impl/vocabulary.h>

namespace rime {

namespace prism {

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t schema_file_checksum;
  uint32_t num_syllables;
  uint32_t num_spellings;
  uint32_t double_array_size;
  OffsetPtr<char> double_array;
};

}

class Prism : public MappedFile {
 public:
  typedef Darts::DoubleArray::result_pair_type Match;

  Prism(const std::string &file_name)
      : MappedFile(file_name), trie_(new Darts::DoubleArray), num_syllables_(0),
      dict_file_checksum_(0), schema_file_checksum_(0) {}

  bool Load();
  bool Save();
  bool Build(const Syllabary &keyset,
             uint32_t dict_file_checksum = 0,
             uint32_t schema_file_checksum = 0);
  
  bool HasKey(const std::string &key);
  bool GetValue(const std::string &key, int *value);
  void CommonPrefixSearch(const std::string &key, std::vector<Match> *result);
  void ExpandSearch(const std::string &key, std::vector<Match> *result, size_t limit);

  size_t array_size() const;

  uint32_t dict_file_checksum() const { return dict_file_checksum_; }
  uint32_t schema_file_checksum() const { return schema_file_checksum_; }

 private:
  scoped_ptr<Darts::DoubleArray> trie_;
  size_t num_syllables_;
  uint32_t dict_file_checksum_;
  uint32_t schema_file_checksum_;
};

}  // namespace rime

#endif  // RIME_PRISM_H_
