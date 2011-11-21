// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
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

namespace rime {

namespace table {

typedef Array<String> Syllabary;

typedef int32_t SyllableId;

typedef List<SyllableId> Code;

struct Entry {
  String text;
  float weight;
};

struct HeadIndexNode {
  List<Entry> entries;
  OffsetPtr<> next_level;
};

typedef Array<HeadIndexNode> HeadIndex;

struct TrunkIndexNode {
  SyllableId key;
  List<Entry> entries;
  OffsetPtr<> next_level;
};

typedef Array<TrunkIndexNode> TrunkIndex;

struct TailIndexNode {
  Code extra_code;
  Entry entry;
};

typedef Array<TailIndexNode> TailIndex;

typedef HeadIndex Index;

struct Metadata {
  static const int kFormatMaxLength = 32;
  char format[kFormatMaxLength];
  uint32_t dict_file_checksum;
  uint32_t num_syllables;
  uint32_t num_entries;
  OffsetPtr<Syllabary> syllabary;
  OffsetPtr<Index> index;
};

}  // namespace table

class TableAccessor {
 public:
  TableAccessor();
  TableAccessor(const Code &index_code, const List<table::Entry> *entries);
  TableAccessor(const Code &index_code, const table::TailIndex *code_map);

  bool Next();

  bool exhausted() const;
  size_t remaining() const;
  const table::Entry* entry() const;
  const table::Code* extra_code() const;
  const Code& index_code() const { return index_code_; }
  const Code code() const;

 private:
  Code index_code_;
  const List<table::Entry> *entries_;
  const table::TailIndex *code_map_;
  size_t cursor_;
};

class TableVisitor {
 public:
  TableVisitor(table::Index *index);

  const TableAccessor Access(int syllable_id) const;

  bool Walk(int syllable_id);  // down to next level
  bool Backdate();  // up one level
  void Reset();  // back to root

  size_t level() const { return level_; }

 private:
  table::HeadIndex *lv1_index_;
  table::TrunkIndex *lv2_index_;
  table::TrunkIndex *lv3_index_;
  table::TailIndex *lv4_index_;
  size_t level_;
  Code index_code_;
};

typedef std::map<int, std::vector<TableAccessor> > TableQueryResult;

struct SyllableGraph;

class Table : public MappedFile {
 public:
  Table(const std::string &file_name)
      : MappedFile(file_name), index_(NULL), syllabary_(NULL), metadata_(NULL) {}

  bool Load();
  bool Save();
  bool Build(const Syllabary &syllabary, const Vocabulary &vocabulary, size_t num_entries,
             uint32_t dict_file_checksum = 0);
  
  bool GetSyllabary(Syllabary *syllabary);
  const char* GetSyllableById(int syllable_id);
  const TableAccessor QueryWords(int syllable_id);
  const TableAccessor QueryPhrases(const Code &code);
  bool Query(const SyllableGraph &syll_graph, size_t start_pos, TableQueryResult *result);
  uint32_t dict_file_checksum() const;

 private:
  table::HeadIndex* BuildHeadIndex(const Vocabulary &vocabulary, size_t num_syllables);
  table::TrunkIndex* BuildTrunkIndex(const Code &prefix, const Vocabulary &vocabulary);
  table::TailIndex* BuildTailIndex(const Code &prefix, const Vocabulary &vocabulary);
  bool BuildEntryList(const DictEntryList &src, List<table::Entry> *dest);
  bool BuildEntry(const DictEntry &dict_entry, table::Entry *entry);

  table::Metadata *metadata_;
  table::Syllabary *syllabary_;
  table::Index *index_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
