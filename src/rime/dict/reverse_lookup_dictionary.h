//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
// 2014-07-06 GONG Chen <chen.sst@gmail.com> redesigned binary file format.
//
#ifndef RIME_REVERSE_LOOKUP_DICTIONARY_H_
#define RIME_REVERSE_LOOKUP_DICTIONARY_H_

#include <stdint.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/string_table.h>
#include <rime/dict/vocabulary.h>

namespace rime {

namespace reverse {

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  String dict_settings;
  List<StringId> index;
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
  explicit ReverseDb(const string& dict_name);

  bool Load();
  bool Lookup(const string& text, string* result);

  bool Build(DictSettings* settings,
             const Syllabary& syllabary,
             const Vocabulary& vocabulary,
             const ReverseLookupTable& stems,
             uint32_t dict_file_checksum);

  uint32_t dict_file_checksum() const;
  reverse::Metadata* metadata() const { return metadata_; }

 private:
  reverse::Metadata* metadata_ = nullptr;
  the<StringTable> key_trie_;
  the<StringTable> value_trie_;
};

class ReverseLookupDictionary
    : public Class<ReverseLookupDictionary, const Ticket&> {
 public:
  explicit ReverseLookupDictionary(an<ReverseDb> db);
  explicit ReverseLookupDictionary(const string& dict_name);
  bool Load();
  bool ReverseLookup(const string& text, string* result);
  bool LookupStems(const string& text, string* result);
  an<DictSettings> GetDictSettings();

 protected:
  an<ReverseDb> db_;
};

class ReverseLookupDictionaryComponent
    : public ReverseLookupDictionary::Component {
 public:
  ReverseLookupDictionaryComponent();
  ReverseLookupDictionary* Create(const Ticket& ticket);
 private:
  map<string, weak<ReverseDb>> db_pool_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_DICTIONARY_H_
