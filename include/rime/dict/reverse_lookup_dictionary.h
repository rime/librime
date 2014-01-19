//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-01-05 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_REVERSE_LOOKUP_DICTIONARY_H_
#define RIME_REVERSE_LOOKUP_DICTIONARY_H_

#include <stdint.h>
#include <map>
#include <string>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/vocabulary.h>

namespace rime {

class ReverseDb : public TreeDb {
 public:
  explicit ReverseDb(const std::string& name);
};

struct Ticket;
class DictSettings;

class ReverseLookupDictionary
    : public Class<ReverseLookupDictionary, const Ticket&> {
 public:
  explicit ReverseLookupDictionary(shared_ptr<ReverseDb> db);
  explicit ReverseLookupDictionary(const std::string& dict_name);
  bool Load();
  bool ReverseLookup(const std::string &text, std::string *result);
  bool LookupStems(const std::string &text, std::string *result);
  bool Build(DictSettings* settings,
             const Syllabary& syllabary,
             const Vocabulary& vocabulary,
             const ReverseLookupTable& stems,
             uint32_t dict_file_checksum);
  uint32_t GetDictFileChecksum();
  shared_ptr<DictSettings> GetDictSettings();
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
