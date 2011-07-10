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
#include <set>
#include <string>
#include <vector>
#include <boost/interprocess/containers/vector.hpp>
#include <rime/common.h>
#include <rime/impl/mapped_file.h>
#include <rime/impl/vocabulary.h>

namespace rime {

namespace table {

typedef boost::interprocess::vector<MappedFile::String,
                                    MappedFile::StringAllocator>
        Syllabary;

struct Entry {
  MappedFile::String text;
  double weight;

  Entry(const MappedFile::VoidAllocator &allocator)
      : text(allocator), weight(0.0) {}
  Entry(const char *_text,
        double _weight,
        const MappedFile::VoidAllocator &allocator)
      : text(_text, allocator), weight(_weight) {}
  // required by move operation
  Entry(const Entry &entry)
      : text(boost::interprocess::move(entry.text)), weight(entry.weight) {}
  Entry& operator=(const Entry &entry) {
    text = boost::interprocess::move(entry.text);
    weight = entry.weight;
  }
};

typedef boost::interprocess::allocator<Entry,
                                       MappedFile::SegmentManager>
        EntryAllocator;
typedef boost::interprocess::vector<Entry,
                                    EntryAllocator>
        EntryVector;
typedef EntryVector::const_iterator EntryIterator;

struct IndexNode {
  boost::interprocess::offset_ptr<void> next_level;
  boost::interprocess::offset_ptr<EntryVector> entries;
  IndexNode()
      : next_level(NULL), entries(NULL) {}
};

typedef boost::interprocess::allocator<IndexNode,
                                       MappedFile::SegmentManager>
        IndexNodeAllocator;
typedef boost::interprocess::vector<IndexNode, IndexNodeAllocator>
        Index;
//typedef boost::interprocess::flat_map<int, IndexNode, IndexNodeAllocator>
//        IndexLv2;
// ...

}  // namespace table

class Table : public MappedFile {
 public:
  Table(const std::string &file_name)
      : MappedFile(file_name), index_(NULL), syllabary_(NULL) {}
  
  bool Load();
  bool Save();
  bool Build(const Syllabary &syllabary, const Vocabulary &vocabulary, size_t num_entries);
  const char* GetSyllableById(int syllable_id);
  const table::EntryVector* GetEntries(int syllable_id);
  
 private:
  table::Index *index_;
  table::Syllabary *syllabary_;
};

}  // namespace rime

#endif  // RIME_TABLE_H_
