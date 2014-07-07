//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
// 2014-07-06 GONG Chen <chen.sst@gmail.com> redesigned binary file format.
//
#ifndef RIME_REVERSE_LOOKUP_DICTIONARY_H_
#define RIME_REVERSE_LOOKUP_DICTIONARY_H_

#include <stdint.h>
#include <map>
#include <string>
#include <darts.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/string_table.h>
#include <rime/dict/vocabulary.h>

namespace rime {

namespace reverse {

using Index = Array<StringId>;

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  String dict_settings;
  OffsetPtr<char> key_trie;
  uint32_t key_trie_size;
  OffsetPtr<char> value_trie;
  uint32_t value_trie_size;
};

}  // namespace reverse

struct Ticket;
class DictSettings;

class ReverseDb : public MappedFile {
 public:
  explicit ReverseDb(const std::string& dict_name);

  bool Load();
  bool Lookup(const std::string& text, std::string* result);

  bool Build(DictSettings* settings,
             const Syllabary& syllabary,
             const Vocabulary& vocabulary,
             const ReverseLookupTable& stems,
             uint32_t dict_file_checksum);

  uint32_t dict_file_checksum() const;
  reverse::Metadata* metadata() const { return metadata_; }

 private:
  reverse::Metadata* metadata_ = nullptr;
  unique_ptr<Darts::DoubleArray> key_trie_;
  unique_ptr<StringTable> value_trie_;
};

class ReverseLookupDictionary
    : public Class<ReverseLookupDictionary, const Ticket&> {
 public:
  explicit ReverseLookupDictionary(shared_ptr<ReverseDb> db);
  explicit ReverseLookupDictionary(const std::string& dict_name);
  bool Load();
  bool ReverseLookup(const std::string& text, std::string* result);
  bool LookupStems(const std::string& text, std::string* result);
  shared_ptr<DictSettings> GetDictSettings();

 protected:
  shared_ptr<ReverseDb> db_;
};

class ReverseLookupDictionaryComponent
    : public ReverseLookupDictionary::Component {
 public:
  ReverseLookupDictionaryComponent();
  ReverseLookupDictionary* Create(const Ticket& ticket);
 private:
  std::map<std::string, weak_ptr<ReverseDb>> db_pool_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_DICTIONARY_H_
