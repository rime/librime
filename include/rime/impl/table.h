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

#include <map>
#include <string>
#include <vector>
#include <boost/interprocess/containers/vector.hpp>
#include <rime/common.h>
#include <rime/impl/mapped_file.h>

namespace rime {
    
class Code : public std::vector<int> {
 public:
  static const size_t kIndexCodeMaxLength = 3;
  
  bool operator< (const Code &other) const;
  bool operator== (const Code &other) const;
};

struct EntryDefinition { 
  std::string text;
  double weight;
  Code code;
  bool operator< (const EntryDefinition& other) const;
};

typedef std::map<Code, std::vector<EntryDefinition> > Vocabulary;

struct TableEntry {
  MappedFile::String text;
  double weight;

  TableEntry(const MappedFile::VoidAllocator &allocator)
      : text(allocator), weight(0.0) {}
  TableEntry(const char *_text,
             double _weight,
             const MappedFile::VoidAllocator &allocator)
      : text(_text, allocator), weight(_weight) {}
  // required by move operation
  TableEntry(const TableEntry &entry)
      : text(boost::interprocess::move(entry.text)), weight(entry.weight) {}
  TableEntry& operator=(const TableEntry &entry) {
    text = boost::interprocess::move(entry.text);
    weight = entry.weight;
  }
};

typedef boost::interprocess::allocator<TableEntry,
                                       MappedFile::SegmentManager>
        TableEntryAllocator;
typedef boost::interprocess::vector<TableEntry,
                                    TableEntryAllocator>
        TableEntryVector;

struct TableIndexNode {
  boost::interprocess::offset_ptr<void> next_level;
  boost::interprocess::offset_ptr<TableEntryVector> entries;
  TableIndexNode()
      : next_level(NULL), entries(NULL) {}
};

typedef boost::interprocess::allocator<TableIndexNode,
                                       MappedFile::SegmentManager>
        TableIndexNodeAllocator;
typedef boost::interprocess::vector<TableIndexNode, TableIndexNodeAllocator>
        TableIndex;
//typedef boost::interprocess::flat_map<int, TableIndexNode, TableIndexNodeAllocator>
//        TableIndexLv2;
// ...

class Table : public MappedFile {
 public:
  Table(const std::string &file_name)
      : MappedFile(file_name), index_(NULL) {}
  
  bool Load();
  bool Save();
  bool Build(const Vocabulary &vocabulary, size_t num_syllables, size_t num_entries);
  const TableEntryVector* GetEntries(int syllable_id);
  
 private:
  TableIndex *index_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
