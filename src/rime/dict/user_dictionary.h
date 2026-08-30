//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DICTIONARY_H_
#define RIME_USER_DICTIONARY_H_

#include <string_view>
#include <time.h>

#ifndef RIME_USER_DICT_CACHE_ENABLED
#define RIME_USER_DICT_CACHE_ENABLED 1
#endif  // RIME_USER_DICT_CACHE_ENABLED
#include <rime_api.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/user_db.h>
#include <rime/dict/vocabulary.h>
#include <unordered_map>

namespace rime {

class RIME_DLL UserDictEntryIterator : public DictEntryFilterBinder {
 public:
  UserDictEntryIterator() = default;

  void Add(an<DictEntry>&& entry);
  void SetEntries(DictEntryList&& entries);
  void SortRange(size_t start, size_t count);

  void AddFilter(DictEntryFilter filter) override;
  an<DictEntry> Peek();
  bool Next();
  bool exhausted() const { return index_ >= cache_.size(); }
  size_t cache_size() const { return cache_.size(); }

 protected:
  bool FindNextEntry();

  DictEntryList cache_;
  size_t index_ = 0;
};

using UserDictEntryCollector = map<size_t, UserDictEntryIterator>;

class Schema;
class Table;
class Prism;
class Db;
struct SyllableGraph;
struct DfsState;
struct Ticket;

class UserDictionary : public Class<UserDictionary, const Ticket&> {
 public:
  RIME_DLL UserDictionary(const string& name, an<Db> db);
  RIME_DLL virtual ~UserDictionary();

  RIME_DLL void Attach(const an<Table>& table, const an<Prism>& prism);
  RIME_DLL bool Load();
  RIME_DLL bool loaded() const;
  RIME_DLL bool readonly() const;

  RIME_DLL an<UserDictEntryCollector> Lookup(
      const SyllableGraph& syllable_graph,
      size_t start_pos,
      size_t depth_limit = 0,
      size_t predict_word_from_depth = 0,
      double initial_credibility = 0.0);
  RIME_DLL size_t LookupWords(UserDictEntryIterator* result,
                              const string& input,
                              bool predictive,
                              size_t limit = 0,
                              string* resume_key = NULL);
  RIME_DLL bool UpdateEntry(const DictEntry& entry, int commits);
  RIME_DLL bool UpdateEntry(const DictEntry& entry,
                            int commits,
                            const string& new_entry_prefix);
  RIME_DLL bool UpdateTickCount(TickCount increment);

  RIME_DLL bool NewTransaction();
  RIME_DLL bool RevertRecentTransaction();
  RIME_DLL bool CommitPendingTransaction();

  // Rebuild the in-memory cache from DB.
  // Call this after any external DB modification (sync/merge/restore).
  RIME_DLL bool Reload();

  const string& name() const { return name_; }
  TickCount tick() const { return tick_; }

  RIME_DLL static an<DictEntry> CreateDictEntry(const string& key,
                                                const string& value,
                                                TickCount present_tick,
                                                double credibility = 0.0,
                                                double quality_len = 0.0,
                                                string* full_code = nullptr);

 protected:
  bool Initialize();
  bool FetchTickCount();
  bool TranslateCodeToString(const Code& code, string* result);
  void DfsLookup(const SyllableGraph& syll_graph,
                 size_t current_pos,
                 const string& current_prefix,
                 DfsState* state);

 private:
  struct CacheEntry {
    std::string_view
        code;  // pinyin code like "ni hao"; points into cache_blob_
    std::string_view text;  // entry text; points into cache_blob_
    double dee;             // difficulty estimate
    int commits;
    TickCount tick;
  };

  struct PendingUpdate {
    enum Type { kAdd, kUpdate, kDelete };
    Type type;
    string code;
    string text;
    double dee;
    int commits;
    TickCount tick;
  };

  string name_;
  an<Db> db_;
  an<Table> table_;
  an<Prism> prism_;
  hash_map<string, SyllableId> syllabary_;
  hash_map<SyllableId, string> rev_syllabary_;
  TickCount tick_ = 0;
  time_t transaction_time_ = 0;

  bool cache_built_ = false;
  TickCount cache_built_tick_ = 0;
  std::string cache_blob_;  // backing storage for CacheEntry code/text views
  vector<CacheEntry> cache_;
  std::unordered_map<string, PendingUpdate> pending_;

  bool BuildCache();
  void CacheLookup(const SyllableGraph& syll_graph,
                   size_t current_pos,
                   const string& current_prefix,
                   DfsState* state);
  void RecruitCacheEntry(const CacheEntry& entry,
                         size_t end_pos,
                         const Code& code,
                         TickCount present_tick,
                         double credibility,
                         double quality_len,
                         hash_map<int, DictEntryList>* result);
  void RecruitPredictiveEntry(const CacheEntry& entry,
                              size_t end_pos,
                              size_t matching_code_size,
                              TickCount present_tick,
                              double credibility,
                              double quality_len,
                              hash_map<int, DictEntryList>* result);
  bool starts_with(std::string_view s, const string& prefix) const;
};

class UserDictionaryComponent : public UserDictionary::Component {
 public:
  UserDictionaryComponent();
  UserDictionary* Create(const Ticket& ticket);
  UserDictionary* Create(const string& dict_name, const string& db_class);

 private:
  hash_map<string, weak<Db>> db_pool_;
};

}  // namespace rime

#endif  // RIME_USER_DICTIONARY_H_
