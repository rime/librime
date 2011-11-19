// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-10-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DICTIONARY_H_
#define RIME_USER_DICTIONRAY_H_

#include <stdint.h>
#include <map>
#include <string>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/impl/vocabulary.h>

namespace rime {

typedef uint64_t TickCount;
  
struct UserDictEntryCollector : std::map<size_t, DictEntryList> {
};

class Schema;
class Table;
class Prism;
class UserDb;
struct SyllableGraph;
struct DfsState;

class UserDictionary : public Class<UserDictionary, Schema*> {
 public:
  explicit UserDictionary(const shared_ptr<UserDb> &user_db);
  virtual ~UserDictionary();

  void Attach(const shared_ptr<Table> &table, const shared_ptr<Prism> &prism);
  bool Load();
  bool loaded() const;

  shared_ptr<UserDictEntryCollector> Lookup(const SyllableGraph &syllable_graph,
                                            size_t start_pos,
                                            size_t depth_limit = 0);
  bool UpdateEntry(const DictEntry &entry, int commit);
  bool UpdateTickCount(TickCount increment);

  const std::string& name() const { return name_; }
  const TickCount tick() const { return tick_; }

 protected:
  bool Initialize();
  bool FetchTickCount();
  bool TranslateCodeToString(const Code &code, std::string *result);
  bool DfsLookup(const SyllableGraph &syll_graph, size_t current_pos,
                 const std::string &current_prefix,
                 DfsState *state);

 private:
  std::string name_;
  TickCount tick_;
  shared_ptr<UserDb> db_;
  shared_ptr<Table> table_;
  shared_ptr<Prism> prism_;
};

class UserDictionaryComponent : public UserDictionary::Component {
 public:
  UserDictionaryComponent();
  UserDictionary* Create(Schema *schema);
 private:
  std::map<std::string, weak_ptr<UserDb> > user_db_pool_;
};

}  // namespace rime

#endif  // RIME_USER_DICTIONRAY_H_
