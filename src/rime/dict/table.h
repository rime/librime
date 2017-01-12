//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-01 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_TABLE_H_
#define RIME_TABLE_H_

#include <cstring>
#include <darts.h>
#include <rime/common.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/vocabulary.h>
#include <rime/dict/string_table.h>

#define RIME_TABLE_UNION(U, V, Ta, a, Tb, b) \
    struct U { \
      V value; \
      const Ta& a() const { return *reinterpret_cast<const Ta*>(this); } \
      const Tb& b() const { return *reinterpret_cast<const Tb*>(this); } \
      Ta& a() { return *reinterpret_cast<Ta*>(this); } \
      Tb& b() { return *reinterpret_cast<Tb*>(this); } \
    }

namespace rime {

namespace table {

//union StringType {
//  String str;
//  StringId str_id;
//};
RIME_TABLE_UNION(StringType, int32_t,
                 String, str,
                 StringId, str_id);

using Syllabary = Array<StringType>;

using Code = List<SyllableId>;

#if defined(__arm__)
using Weight = double;
#else
using Weight = float;
#endif

struct Entry {
  StringType text;
  Weight weight;
};

struct LongEntry {
  Code extra_code;
  Entry entry;
};

struct PhraseIndex;

struct HeadIndexNode {
  List<Entry> entries;
  OffsetPtr<PhraseIndex> next_level;
};

using HeadIndex = Array<HeadIndexNode>;

struct TrunkIndexNode {
  SyllableId key;
  List<Entry> entries;
  OffsetPtr<PhraseIndex> next_level;
};

using TrunkIndex = Array<TrunkIndexNode>;

using TailIndex = Array<LongEntry>;

//union Phraseindex {
//  TrunkIndex trunk;
//  TailIndex tail;
//};
RIME_TABLE_UNION(PhraseIndex, Array<char>,
                 TrunkIndex, trunk,
                 TailIndex, tail);

using Index = HeadIndex;

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t num_syllables;
  uint32_t num_entries;
  OffsetPtr<Syllabary> syllabary;
  OffsetPtr<Index> index;
  // v2
  int32_t reserved_1;
  int32_t reserved_2;
  OffsetPtr<char> string_table;
  uint32_t string_table_size;
};

}  // namespace table

class TableAccessor {
 public:
  TableAccessor() = default;
  TableAccessor(const Code& index_code, const List<table::Entry>* entries,
                double credibility = 1.0);
  TableAccessor(const Code& index_code, const Array<table::Entry>* entries,
                double credibility = 1.0);
  TableAccessor(const Code& index_code, const table::TailIndex* code_map,
                double credibility = 1.0);

  bool Next();

  bool exhausted() const;
  size_t remaining() const;
  const table::Entry* entry() const;
  const table::Code* extra_code() const;
  const Code& index_code() const { return index_code_; }
  Code code() const;
  double credibility() const { return credibility_; }

 private:
  Code index_code_;
  const table::Entry* entries_ = nullptr;
  const table::LongEntry* long_entries_ = nullptr;
  size_t size_ = 0;
  size_t cursor_ = 0;
  double credibility_ = 1.0;
};

using TableQueryResult = map<int, vector<TableAccessor>>;

struct SyllableGraph;
class TableQuery;

class Table : public MappedFile {
 public:
  Table(const string& file_name);
  virtual ~Table();

  bool Load();
  bool Save();
  bool Build(const Syllabary& syllabary,
             const Vocabulary& vocabulary,
             size_t num_entries,
             uint32_t dict_file_checksum = 0);

  bool GetSyllabary(Syllabary* syllabary);
  string GetSyllableById(int syllable_id);
  TableAccessor QueryWords(int syllable_id);
  TableAccessor QueryPhrases(const Code& code);
  bool Query(const SyllableGraph& syll_graph,
             size_t start_pos,
             TableQueryResult* result);
  string GetEntryText(const table::Entry& entry);

  uint32_t dict_file_checksum() const;

 private:
  table::Index* BuildIndex(const Vocabulary& vocabulary,
                           size_t num_syllables);
  table::HeadIndex* BuildHeadIndex(const Vocabulary& vocabulary,
                                   size_t num_syllables);
  table::TrunkIndex* BuildTrunkIndex(const Code& prefix,
                                     const Vocabulary& vocabulary);
  table::TailIndex* BuildTailIndex(const Code& prefix,
                                   const Vocabulary& vocabulary);
  bool BuildPhraseIndex(Code code, const Vocabulary& vocabulary,
                        map<string, int>* index_data);
  Array<table::Entry>* BuildEntryArray(const DictEntryList& entries);
  bool BuildEntryList(const DictEntryList& src, List<table::Entry>* dest);
  bool BuildEntry(const DictEntry& dict_entry, table::Entry* entry);

  string GetString_v1(const table::StringType& x);
  bool AddString_v1(const string& src, table::StringType* dest,
                    double weight);

  // v2
  string GetString_v2(const table::StringType& x);
  bool AddString_v2(const string& src, table::StringType* dest,
                    double weight);
  bool OnBuildStart_v2();
  bool OnBuildFinish_v2();
  bool OnLoad_v2();

  void SelectTableFormat(double format_version);

 protected:
  table::Metadata* metadata_ = nullptr;
  table::Syllabary* syllabary_ = nullptr;
  table::Index* index_ = nullptr;

  struct TableFormat {
    const char* format_name;

    string (Table::*GetString)(const table::StringType& x);
    bool (Table::*AddString)(const string& src, table::StringType* dest,
                             double weight);

    bool (Table::*OnBuildStart)();
    bool (Table::*OnBuildFinish)();
    bool (Table::*OnLoad)();
  } format_;

  // v2
  the<StringTable> string_table_;
  the<StringTableBuilder> string_table_builder_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
