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

struct Chunk;
struct QueryResult;

}  // namespace dictionary

class RIME_API DictEntryIterator : public DictEntryFilterBinder {
 public:
  DictEntryIterator();
  virtual ~DictEntryIterator() = default;
  DictEntryIterator(const DictEntryIterator& other) = default;
  DictEntryIterator& operator=(const DictEntryIterator& other) = default;
  DictEntryIterator(DictEntryIterator&& other) = default;
  DictEntryIterator& operator=(DictEntryIterator&& other) = default;

  void AddChunk(dictionary::Chunk&& chunk);
  void Sort();
  void AddFilter(DictEntryFilter filter) override;
  an<DictEntry> Peek();
  bool Next();
  bool Skip(size_t num_entries);
  bool exhausted() const;
  size_t entry_count() const { return entry_count_; }

 protected:
  bool FindNextEntry();

 private:
  an<dictionary::QueryResult> query_result_;
  size_t chunk_index_ = 0;
  an<DictEntry> entry_ = nullptr;
  size_t entry_count_ = 0;
};

using DictEntryCollector = map<size_t, DictEntryIterator>;

class Config;
class Schema;
class EditDistanceCorrector;
struct SyllableGraph;
struct Ticket;

class Dictionary : public Class<Dictionary, const Ticket&> {
 public:
  RIME_API Dictionary(string name,
                      vector<string> packs,
                      vector<of<Table>> tables,
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
                              bool predictive,
                              size_t limit = 0);
  // translate syllable id sequence to string code
  RIME_API bool Decode(const Code& code, vector<string>* result);

  const string& name() const { return name_; }
  RIME_API bool loaded() const;

  const vector<string>& packs() const { return packs_; }
  const vector<of<Table>>& tables() const { return tables_; }
  const an<Table>& primary_table() const { return tables_[0]; }
  const an<Prism>& prism() const { return prism_; }

 private:
  string name_;
  vector<string> packs_;
  vector<of<Table>> tables_;
  an<Prism> prism_;
};

class ResourceResolver;

class DictionaryComponent : public Dictionary::Component {
 public:
  DictionaryComponent();
  ~DictionaryComponent() override;
  Dictionary* Create(const Ticket& ticket) override;
  Dictionary* Create(string dict_name, string prism_name, vector<string> packs);

 private:
  map<string, weak<Prism>> prism_map_;
  map<string, weak<Table>> table_map_;
  the<ResourceResolver> prism_resource_resolver_;
  the<ResourceResolver> table_resource_resolver_;
};

}  // namespace rime

#endif  // RIME_DICTIONARY_H_
