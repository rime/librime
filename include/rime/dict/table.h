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
#include <rime/common.h>
#include <rime/dict/mapped_file.h>
#include <rime/dict/vocabulary.h>
#include <rime/dict/string_table.h>

namespace rime {

namespace table {

using Syllabary = Array<String>;

using SyllableId = int32_t;

using Code = List<SyllableId>;

#if defined(__arm__)
using Weight = double;
#else
using Weight = float;
#endif

struct Entry {
  union {
    String str;
    StringId str_id;
  } text;
  Weight weight;
};

struct HeadIndexNode {
  List<Entry> entries;
  OffsetPtr<> next_level;
};

using HeadIndex = Array<HeadIndexNode>;

struct TrunkIndexNode {
  SyllableId key;
  List<Entry> entries;
  OffsetPtr<> next_level;
};

using TrunkIndex = Array<TrunkIndexNode>;

struct TailIndexNode {
  Code extra_code;
  Entry entry;
};

using TailIndex = Array<TailIndexNode>;

using Index = HeadIndex;

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t num_syllables;
  uint32_t num_entries;
  OffsetPtr<Syllabary> syllabary;
  OffsetPtr<Index> index;
  // v1.1
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

class TableVisitor {
 public:
  TableVisitor(table::Index* index);

  TableAccessor Access(int syllable_id,
                       double credibility = 1.0) const;

  // down to next level
  bool Walk(int syllable_id, double credibility = 1.0);
  // up one level
  bool Backdate();
  // back to root
  void Reset();

  size_t level() const { return level_; }

 private:
  table::HeadIndex* lv1_index_ = nullptr;
  table::TrunkIndex* lv2_index_ = nullptr;
  table::TrunkIndex* lv3_index_ = nullptr;
  table::TailIndex* lv4_index_ = nullptr;
  size_t level_ = 0;
  Code index_code_;
  std::vector<double> credibility_;
};

using TableQueryResult = std::map<int, std::vector<TableAccessor>>;

struct SyllableGraph;

class Table : public MappedFile {
 public:
  Table(const std::string& file_name)
      : MappedFile(file_name) {}

  bool Load();
  bool Save();
  bool Build(const Syllabary& syllabary,
             const Vocabulary& vocabulary,
             size_t num_entries,
             uint32_t dict_file_checksum = 0);

  bool GetSyllabary(Syllabary* syllabary);
  const char* GetSyllableById(int syllable_id);
  TableAccessor QueryWords(int syllable_id);
  TableAccessor QueryPhrases(const Code& code);
  bool Query(const SyllableGraph& syll_graph,
             size_t start_pos,
             TableQueryResult* result);
  std::string GetEntryText(const table::Entry& entry);

  uint32_t dict_file_checksum() const;

 private:
  table::HeadIndex* BuildHeadIndex(const Vocabulary& vocabulary,
                                   size_t num_syllables);
  table::TrunkIndex* BuildTrunkIndex(const Code& prefix,
                                     const Vocabulary& vocabulary);
  table::TailIndex* BuildTailIndex(const Code& prefix,
                                   const Vocabulary& vocabulary);
  bool BuildEntryList(const DictEntryList& src, List<table::Entry>* dest);
  bool BuildEntry(const DictEntry& dict_entry, table::Entry* entry);

  table::Index* index_ = nullptr;
  table::Syllabary* syllabary_ = nullptr;
  table::Metadata* metadata_ = nullptr;

  bool use_string_table_ = true;
  unique_ptr<StringTable> string_table_;
  unique_ptr<StringTableBuilder> string_table_builder_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
