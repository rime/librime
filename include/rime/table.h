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

#include <boost/interprocess/containers/vector.hpp>
#include <rime/common.h>
#include <rime/mapped_file.h>

namespace rime {

class TableEntry {
  MappedFile::String text_;
  double weight_;

 public:
  TableEntry(const MappedFile::VoidAllocator &allocator)
      : text_(allocator), weight_(0.0) {}
  TableEntry(const char *text,
             double weight,
             const MappedFile::VoidAllocator &allocator)
      : text_(text, allocator), weight_(weight) {}

  const MappedFile::String &text() const { return text_; }
  double weight() const { return weight_; }
};

typedef boost::interprocess::allocator<TableEntry,
                                       MappedFile::SegmentManager>
        TableEntryAllocator;
typedef boost::interprocess::vector<TableEntry,
                                    TableEntryAllocator>
        TableEntryVector;

struct TableIndexNode {
  boost::interprocess::offset_ptr<void> next_level;
  TableEntryVector entries;
};

typedef boost::interprocess::allocator<TableIndexNode,
                                       MappedFile::SegmentManager>
        TableIndexNodeAllocator;
typedef boost::interprocess::vector<TableIndexNode, TableIndexNodeAllocator>
        TableIndex;
//typedef boost::interprocess::flat_map<int, TableIndexNode, TableIndexNodeAllocator>
//        TableIndexL2;
// ...

class Table : public MappedFile {
 public:
  Table(const std::string &file_name)
      : MappedFile(file_name), index_(NULL) {}
  
  bool Load();
  bool Save();
  void Build(/* arguments to argue */);
  const TableEntryVector* GetEntries(int syllable_id);
  
 private:
  TableIndex *index_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
