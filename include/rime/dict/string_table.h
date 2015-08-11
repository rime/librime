//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-06-25 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_STRING_TABLE_H_
#define RIME_STRING_TABLE_H_

#include <utility>
#include <marisa.h>
#include <rime/common.h>

namespace rime {

using StringId = marisa::UInt32;

const StringId kInvalidStringId = (StringId)(-1);

class StringTable {
 public:
  StringTable() = default;
  virtual ~StringTable() = default;
  StringTable(const char* ptr, size_t size);

  bool HasKey(const string& key);
  StringId Lookup(const string& key);
  void CommonPrefixMatch(const string& query,
                         vector<StringId>* result);
  void Predict(const string& query,
               vector<StringId>* result);
  string GetString(StringId string_id);

  size_t NumKeys() const;
  size_t BinarySize() const;

 protected:
  marisa::Trie trie_;
};

class StringTableBuilder: public StringTable {
 public:
  void Add(const string& key, double weight = 1.0,
           StringId* reference = nullptr);
  void Clear();
  void Build();
  void Dump(char* ptr, size_t size);

 private:
  void UpdateReferences();

  marisa::Keyset keys_;
  vector<StringId*> references_;
};

}  // namespace rime

#endif  // RIME_STRING_TABLE_H_
