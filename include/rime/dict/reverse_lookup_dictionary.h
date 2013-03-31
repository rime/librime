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

class ReverseLookupDictionary
    : public Class<ReverseLookupDictionary, const Ticket&> {
 public:
  explicit ReverseLookupDictionary(const shared_ptr<TreeDb> &db);
  bool Load();
  bool ReverseLookup(const std::string &text, std::string *result);
 protected:
  shared_ptr<TreeDb> db_;
};

class ReverseLookupDictionaryComponent
    : public ReverseLookupDictionary::Component {
 public:
  ReverseLookupDictionaryComponent();
  ReverseLookupDictionary* Create(const Ticket& ticket);
 private:
  std::map<std::string, weak_ptr<TreeDb> > db_pool_;
};

}  // namespace rime

#endif  // RIME_REVERSE_LOOKUP_DICTIONARY_H_
