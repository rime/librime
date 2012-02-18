// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICTIONARY_H_
#define RIME_DICTIONARY_H_

#include <list>
#include <map>
#include <string>
#include <vector>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/vocabulary.h>

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

struct Chunk {
  Code code;
  const table::Entry *entries;
  size_t size;
  size_t cursor;
  std::string remaining_code;  // for predictive queries
  double credibility;

  Chunk() : entries(NULL), size(0), cursor(0), credibility(1.0) {}
  Chunk(const Code &c, const table::Entry *e, double cr = 1.0)
      : code(c), entries(e), size(1), cursor(0), credibility(cr) {}
  Chunk(const TableAccessor &a, double cr = 1.0)
      : code(a.index_code()), entries(a.entry()),
        size(a.remaining()), cursor(0), credibility(cr) {}
  Chunk(const TableAccessor &a, const std::string &r, double cr = 1.0)
      : code(a.index_code()), entries(a.entry()),
        size(a.remaining()), cursor(0), remaining_code(r), credibility(cr) {}
};

bool compare_chunk_by_leading_element(const Chunk &a, const Chunk &b);

}  // namespace dictionary

class DictEntryIterator : protected std::list<dictionary::Chunk> {
 public:
  typedef std::list<dictionary::Chunk> Base;

  DictEntryIterator();
  DictEntryIterator(const DictEntryIterator &other);
  DictEntryIterator& operator= (DictEntryIterator &other);

  void AddChunk(const dictionary::Chunk &chunk);
  void Sort();
  shared_ptr<DictEntry> Peek();
  bool Next();
  bool Skip(size_t num_entries);
  bool exhausted() const;
  size_t entry_count() const { return entry_count_; }

private:
  shared_ptr<DictEntry> entry_;
  size_t entry_count_;
};

struct DictEntryCollector : std::map<size_t, DictEntryIterator> {
};

class Config;
class Schema;
struct SyllableGraph;

class Dictionary : public Class<Dictionary, Schema*> {
 public:
  Dictionary(const std::string &name,
             const shared_ptr<Table> &table,
             const shared_ptr<Prism> &prism);
  virtual ~Dictionary();

  bool Exists() const;
  bool Remove();
  bool Load();

  shared_ptr<DictEntryCollector> Lookup(const SyllableGraph &syllable_graph,
                                        size_t start_pos,
                                        double initial_credibility = 1.0);
  // if predictive is true, do an expand search with limit,
  // otherwise do an exact match.
  // return num of matching keys.
  size_t LookupWords(DictEntryIterator *result,
                     const std::string &str_code,
                     bool predictive, size_t limit = 0);
  // translate syllable id sequence to string code
  bool Decode(const Code &code, dictionary::RawCode *result);

  const std::string& name() const { return name_; }
  bool loaded() const;
  
  shared_ptr<Table> table() { return table_; }
  shared_ptr<Prism> prism() { return prism_; }

 private:
  std::string name_;
  shared_ptr<Table> table_;
  shared_ptr<Prism> prism_;
};

class DictionaryComponent : public Dictionary::Component {
 public:
  DictionaryComponent();
  Dictionary* Create(Schema *schema);
  Dictionary* CreateDictionaryFromConfig(Config *config,
                                         const std::string &customer);
  Dictionary* CreateDictionaryWithName(const std::string &dict_name,
                                       const std::string &prism_name);

 private:
  std::map<std::string, weak_ptr<Prism> > prism_map_;
  std::map<std::string, weak_ptr<Table> > table_map_;
};

}  // namespace rime

#endif  // RIME_DICTIONRAY_H_
