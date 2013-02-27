//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DICTIONARY_H_
#define RIME_USER_DICTIONARY_H_

#include <stdint.h>
#include <time.h>
#include <map>
#include <string>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/vocabulary.h>

namespace rime {

typedef uint64_t TickCount;

struct UserDictEntryCollector : std::map<size_t, DictEntryList> {
};

class UserDictEntryIterator {
 public:
  UserDictEntryIterator() : entries_(), index_(0) {}
  
  void Add(const shared_ptr<DictEntry>& entry);
  void SortN(size_t count);
  bool Release(DictEntryList* receiver);
  
  shared_ptr<DictEntry> Peek();
  bool Next();
  bool exhausted() const {
    return !entries_ || index_ >= entries_->size();
  }
  
 protected:
  shared_ptr<DictEntryList> entries_;
  size_t index_;
};

class Schema;
class Table;
class Prism;
class UserDb;
struct SyllableGraph;
struct DfsState;
struct Ticket;

class UserDictionary : public Class<UserDictionary, const Ticket&> {
 public:
  explicit UserDictionary(const shared_ptr<UserDb> &user_db);
  virtual ~UserDictionary();

  void Attach(const shared_ptr<Table> &table, const shared_ptr<Prism> &prism);
  bool Load();
  bool loaded() const;

  shared_ptr<UserDictEntryCollector> Lookup(const SyllableGraph &syllable_graph,
                                            size_t start_pos,
                                            size_t depth_limit = 0,
                                            double initial_credibility = 1.0);
  size_t LookupWords(UserDictEntryIterator* result,
                     const std::string& input,
                     bool predictive,
                     size_t limit = 0,
                     std::string* resume_key = NULL);
  bool UpdateEntry(const DictEntry &entry, int commit);
  bool UpdateTickCount(TickCount increment);

  bool NewTransaction();
  bool RevertRecentTransaction();
  bool CommitPendingTransaction();

  const std::string& name() const { return name_; }
  const TickCount tick() const { return tick_; }

  static shared_ptr<DictEntry> CreateDictEntry(const std::string& key,
                                               const std::string& value,
                                               TickCount present_tick,
                                               double credibility = 1.0,
                                               std::string* full_code = NULL);
  static bool UnpackValues(const std::string &value,
                           int *commit_count, double *dee, TickCount *tick);

 protected:
  bool Initialize();
  bool FetchTickCount();
  bool TranslateCodeToString(const Code &code, std::string *result);
  void DfsLookup(const SyllableGraph &syll_graph, size_t current_pos,
                 const std::string &current_prefix,
                 DfsState *state);

 private:
  std::string name_;
  shared_ptr<UserDb> db_;
  shared_ptr<Table> table_;
  shared_ptr<Prism> prism_;
  TickCount tick_;
  time_t transaction_time_;
};

class UserDictionaryComponent : public UserDictionary::Component {
 public:
  UserDictionaryComponent();
  UserDictionary* Create(const Ticket& ticket);
 private:
  std::map<std::string, weak_ptr<UserDb> > db_pool_;
};

}  // namespace rime

#endif  // RIME_USER_DICTIONARY_H_
