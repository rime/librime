//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_DICTIONARY_H_
#define RIME_REVERSE_LOOKUP_DICTIONARY_H_

#include <map>
#include <string>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/user_db.h>

namespace rime {

struct Ticket;

class ReverseDb : public TreeDb {
 public:
  explicit ReverseDb(const std::string& name)
      : TreeDb(name + ".reverse.bin", "reversedb") {
  }
};

class ReverseLookupDictionary
    : public Class<ReverseLookupDictionary, const Ticket&> {
 public:
  explicit ReverseLookupDictionary(shared_ptr<ReverseDb> db);
  bool Load();
  bool ReverseLookup(const std::string &text, std::string *result);
 protected:
  shared_ptr<ReverseDb> db_;
};

class ReverseLookupDictionaryComponent
    : public ReverseLookupDictionary::Component {
 public:
  ReverseLookupDictionaryComponent();
  ReverseLookupDictionary* Create(const Ticket& ticket);
 private:
  std::map<std::string, weak_ptr<ReverseDb> > db_pool_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_DICTIONARY_H_
