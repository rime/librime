//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-07-01 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_TABLE_H_
#define RIME_TABLE_H_

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <darts.h>
#include <rime/common.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/vocabulary.h>
#include <rime/dict/string_table.h>

namespace rime {

namespace table {

union StringType {
  String str;
  StringId str_id;
};

using Syllabary = Array<StringType>;

using SyllableId = int32_t;

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

union PhraseIndex;

struct HeadIndexNode_v1 {
  List<Entry> entries;
  OffsetPtr<PhraseIndex> next_level;
};

using HeadIndex_v1 = Array<HeadIndexNode_v1>;

struct TrunkIndexNode_v1 {
  SyllableId key;
  List<Entry> entries;
  OffsetPtr<PhraseIndex> next_level;
};

using TrunkIndex_v1 = Array<TrunkIndexNode_v1>;

using HeadIndex_v2 = Array<List<Entry>>;

using TrunkIndex_v2 = Array<Entry>;

using TailIndex = Array<LongEntry>;

union PhraseIndex {
  TrunkIndex_v1 trunk_v1;
  TrunkIndex_v2 trunk_v2;
  TailIndex tail;
};

union Index {
  HeadIndex_v1 v1;
  HeadIndex_v2 v2;
};

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t num_syllables;
  uint32_t num_entries;
  OffsetPtr<Syllabary> syllabary;
  OffsetPtr<Index> index;
  // v2
  OffsetPtr<char> trie;
  OffsetPtr<char> string_table;
  uint32_t string_table_size;
};

}  // namespace table

class TableAccessor {
 public:
  TableAccessor() = default;
  TableAccessor(const Code& index_code, const List<table::Entry>* entries,
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
  const List<table::Entry>* entries_ = nullptr;
  const table::TailIndex* code_map_ = nullptr;
  size_t cursor_ = 0;
  double credibility_ = 1.0;
};

using TableQueryResult = std::map<int, std::vector<TableAccessor>>;

struct SyllableGraph;
class TableQuery;

class Table : public MappedFile {
 public:
  Table(const std::string& file_name);
  virtual ~Table();

  bool Load();
  bool Save();
  bool Build(const Syllabary& syllabary,
             const Vocabulary& vocabulary,
             size_t num_entries,
             uint32_t dict_file_checksum = 0);

  bool GetSyllabary(Syllabary* syllabary);
  std::string GetSyllableById(int syllable_id);
  TableAccessor QueryWords(int syllable_id);
  TableAccessor QueryPhrases(const Code& code);
  bool Query(const SyllableGraph& syll_graph,
             size_t start_pos,
             TableQueryResult* result);
  std::string GetEntryText(const table::Entry& entry);

  uint32_t dict_file_checksum() const;

 private:
  table::HeadIndex_v1* BuildHeadIndex(const Vocabulary& vocabulary,
                                      size_t num_syllables);
  table::TrunkIndex_v1* BuildTrunkIndex(const Code& prefix,
                                        const Vocabulary& vocabulary);
  table::TailIndex* BuildTailIndex(const Code& prefix,
                                   const Vocabulary& vocabulary);
  bool BuildEntryList(const DictEntryList& src, List<table::Entry>* dest);
  bool BuildEntry(const DictEntry& dict_entry, table::Entry* entry);

  TableQuery* NewQuery_v1();
  std::string GetString_v1(const table::StringType& x);
  bool AddString_v1(const std::string& src, table::StringType* dest,
                    double weight);
  table::Index* BuildIndex_v1(const Vocabulary& vocabulary,
                              size_t num_syllables);

  // v2
  TableQuery* NewQuery_v2();
  std::string GetString_v2(const table::StringType& x);
  bool AddString_v2(const std::string& src, table::StringType* dest,
                    double weight);
  table::Index* BuildIndex_v2(const Vocabulary& vocabulary,
                              size_t num_syllables);
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

    TableQuery* (Table::*NewQuery)();
    std::string (Table::*GetString)(const table::StringType& x);
    bool (Table::*AddString)(const std::string& src, table::StringType* dest,
                             double weight);
    table::Index* (Table::*BuildIndex)(const Vocabulary& vocabulary,
                                       size_t num_syllables);

    bool (Table::*OnBuildStart)();
    bool (Table::*OnBuildFinish)();
    bool (Table::*OnLoad)();
  } format_;

  // v2
  unique_ptr<Darts::DoubleArray> trie_;
  unique_ptr<StringTable> string_table_;
  unique_ptr<StringTableBuilder> string_table_builder_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
