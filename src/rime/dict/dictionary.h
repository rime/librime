//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DICTIONARY_H_
#define RIME_DICTIONARY_H_

#include <rime_api.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/prism.h>
#include <rime/dict/table.h>
#include <rime/dict/vocabulary.h>

namespace rime {

namespace dictionary {

struct Chunk {
  Code code;
  const table::Entry* entries = nullptr;
  size_t size = 0;
  size_t cursor = 0;
  string remaining_code;  // for predictive queries
  double credibility = 0.0;

  Chunk() = default;
  Chunk(const Code& c, const table::Entry* e, double cr = 0.0)
      : code(c), entries(e), size(1), cursor(0), credibility(cr) {}
  Chunk(const TableAccessor& a, double cr = 0.0)
      : Chunk(a, string(), cr) {}
  Chunk(const TableAccessor& a, const string& r, double cr = 0.0)
      : code(a.index_code()), entries(a.entry()),
        size(a.remaining()), cursor(0), remaining_code(r), credibility(cr) {}
};

bool compare_chunk_by_leading_element(const Chunk& a, const Chunk& b);

}  // namespace dictionary

class DictEntryIterator : public DictEntryFilterBinder {
 public:
  DictEntryIterator() = default;
  DictEntryIterator(DictEntryIterator&& other) = default;
  DictEntryIterator& operator= (DictEntryIterator&& other) = default;
  DictEntryIterator(const DictEntryIterator& other) = default;
  DictEntryIterator& operator= (const DictEntryIterator& other) = default;

  void AddChunk(dictionary::Chunk&& chunk, Table* table);
  void Sort();
  RIME_API void AddFilter(DictEntryFilter filter) override;
  RIME_API an<DictEntry> Peek();
  RIME_API bool Next();
  bool Skip(size_t num_entries);
  bool exhausted() const { return chunk_index_ >= chunks_.size(); }
  size_t entry_count() const { return entry_count_; }

 protected:
  bool FindNextEntry();

 private:
  vector<dictionary::Chunk> chunks_;
  size_t chunk_index_ = 0;
  Table* table_ = nullptr;
  an<DictEntry> entry_ = nullptr;
  size_t entry_count_ = 0;
};

struct DictEntryCollector : map<size_t, DictEntryIterator> {
};

class Config;
class Schema;
class EditDistanceCorrector;
struct SyllableGraph;
struct Ticket;

class Dictionary : public Class<Dictionary, const Ticket&> {
 public:
  RIME_API Dictionary(const string& name,
                      an<Table> table,
                      an<Prism> prism);
  virtual ~Dictionary();

  bool Exists() const;
  RIME_API bool Remove();
  RIME_API bool Load();

  RIME_API an<DictEntryCollector> Lookup(const SyllableGraph& syllable_graph,
                                         size_t start_pos,
                                         double initial_credibility = 0.0);
  // if predictive is true, do an expand search with limit,
  // otherwise do an exact match.
  // return num of matching keys.
  RIME_API size_t LookupWords(DictEntryIterator* result,
                              const string& str_code,
                              bool predictive, size_t limit = 0);
  // translate syllable id sequence to string code
  RIME_API bool Decode(const Code& code, vector<string>* result);

  const string& name() const { return name_; }
  RIME_API bool loaded() const;

  an<Table> table() { return table_; }
  an<Prism> prism() { return prism_; }

 private:
  string name_;
  an<Table> table_;
  an<Prism> prism_;
};

class ResourceResolver;

class DictionaryComponent : public Dictionary::Component {
 public:
  DictionaryComponent();
  ~DictionaryComponent() override;
  Dictionary* Create(const Ticket& ticket) override;
  Dictionary* CreateDictionaryWithName(const string& dict_name,
                                       const string& prism_name);

 private:
  map<string, weak<Prism>> prism_map_;
  map<string, weak<Table>> table_map_;
  the<ResourceResolver> prism_resource_resolver_;
  the<ResourceResolver> table_resource_resolver_;
};

}  // namespace rime

#endif  // RIME_DICTIONARY_H_
