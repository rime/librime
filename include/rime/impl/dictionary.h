// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICTIONARY_H_
#define RIME_DICTIONRAY_H_

#include <list>
#include <string>
#include <vector>
#include <rime/common.h>
#include <rime/impl/prism.h>
#include <rime/impl/table.h>
#include <rime/impl/vocabulary.h>

namespace rime {

namespace dictionary {

class RawCode : public std::vector<std::string> {
 public:
  const std::string ToString() const;
  void FromString(const std::string &str_code);
};

struct RawDictEntry {
  RawCode raw_code;
  std::string text;
  double weight;
};

typedef std::list<rime::shared_ptr<RawDictEntry> > RawDictEntryList;

struct Chunk {
  Code code;
  const table::Entry *entries;
  size_t size;
  size_t cursor;
  std::string remaining_code;  // for predictive queries

  Chunk() : entries(NULL), size(0), cursor(0) {}
  Chunk(const Code &c, const table::Entry *e)
      : code(c), entries(e), size(1), cursor(0) {}
  Chunk(const TableAccessor &a)
      : code(a.index_code()), entries(a.entry()),
        size(a.remaining()), cursor(0) {}
  Chunk(const TableAccessor &a, const std::string &r)
      : code(a.index_code()), entries(a.entry()),
        size(a.remaining()), cursor(0), remaining_code(r) {}
};

}  // namespace dictionary

class DictEntryIterator : public std::list<dictionary::Chunk> {
 public:
  typedef std::list<dictionary::Chunk> Base;

  DictEntryIterator();
  DictEntryIterator(const DictEntryIterator &other);

  shared_ptr<DictEntry> Peek();
  bool Next();
  bool exhausted() const;

private:
  shared_ptr<DictEntry> entry_;
};

class DictEntryCollector : public std::map<int, DictEntryIterator> {
};

struct SyllableGraph;

class Dictionary {
 public:
  Dictionary(const std::string &name);
  virtual ~Dictionary();

  bool Exists() const;
  bool Compile(const std::string &source_file);
  bool Remove();
  bool Load();
  bool Unload();

  shared_ptr<DictEntryCollector> Lookup(const SyllableGraph &syllable_graph, int start_pos);
  DictEntryIterator LookupWords(const std::string &str_code, bool predictive);
  bool Decode(const Code &code, dictionary::RawCode *result);

  const std::string& name() const { return name_; }
  bool loaded() const { return loaded_; }
  // limited operations
  Prism* prism() { return prism_.get(); }

 private:
  std::string name_;
  bool loaded_;
  scoped_ptr<Prism> prism_;
  scoped_ptr<Table> table_;
};

}  // namespace rime

#endif  // RIME_DICTIONRAY_H_
