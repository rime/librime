//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DICTIONARY_H_
#define RIME_USER_DICTIONARY_H_

#include <time.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/user_db.h>
#include <rime/dict/vocabulary.h>

namespace rime {

struct UserDictEntryCollector : map<size_t, DictEntryList> {
};

class UserDictEntryIterator : public DictEntryFilterBinder {
 public:
  UserDictEntryIterator() = default;

  void Add(const an<DictEntry>& entry);
  void SortRange(size_t start, size_t count);
  bool Release(DictEntryList* receiver);

  an<DictEntry> Peek();
  bool Next();
  bool exhausted() const {
    return !entries_ || index_ >= entries_->size();
  }
  size_t size() const {
    return entries_ ? entries_->size() : 0;
  }

 protected:
  an<DictEntryList> entries_;
  size_t index_ = 0;
};

class Schema;
class Table;
class Prism;
class Db;
struct SyllableGraph;
struct DfsState;
struct Ticket;

class UserDictionary : public Class<UserDictionary, const Ticket&> {
 public:
  explicit UserDictionary(const an<Db>& db);
  virtual ~UserDictionary();

  void Attach(const an<Table>& table, const an<Prism>& prism);
  bool Load();
  bool loaded() const;
  bool readonly() const;

  an<UserDictEntryCollector> Lookup(const SyllableGraph& syllable_graph,
                                            size_t start_pos,
                                            size_t depth_limit = 0,
                                            double initial_credibility = 1.0);
  size_t LookupWords(UserDictEntryIterator* result,
                     const string& input,
                     bool predictive,
                     size_t limit = 0,
                     string* resume_key = NULL);
  bool UpdateEntry(const DictEntry& entry, int commits);
  bool UpdateEntry(const DictEntry& entry, int commits,
                   const string& new_entry_prefix);
  bool UpdateTickCount(TickCount increment);

  bool NewTransaction();
  bool RevertRecentTransaction();
  bool CommitPendingTransaction();

  const string& name() const { return name_; }
  TickCount tick() const { return tick_; }

  static an<DictEntry> CreateDictEntry(const string& key,
                                               const string& value,
                                               TickCount present_tick,
                                               double credibility = 1.0,
                                               string* full_code = NULL);

 protected:
  bool Initialize();
  bool FetchTickCount();
  bool TranslateCodeToString(const Code& code, string* result);
  void DfsLookup(const SyllableGraph& syll_graph, size_t current_pos,
                 const string& current_prefix,
                 DfsState* state);

 private:
  string name_;
  an<Db> db_;
  an<Table> table_;
  an<Prism> prism_;
  TickCount tick_ = 0;
  time_t transaction_time_ = 0;
};

class UserDictionaryComponent : public UserDictionary::Component {
 public:
  UserDictionaryComponent();
  UserDictionary* Create(const Ticket& ticket);
 private:
  map<string, weak<Db>> db_pool_;
};

}  // namespace rime

#endif  // RIME_USER_DICTIONARY_H_
