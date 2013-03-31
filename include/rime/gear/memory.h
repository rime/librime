//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-01-02 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MEMORY_H_
#define RIME_MEMORY_H_

#include <string>
#include <vector>
#include <boost/signals/connection.hpp>
#include <rime/common.h>
#include <rime/dict/vocabulary.h>

namespace rime {

class KeyEvent;
class Context;
class Engine;
class Dictionary;
class UserDictionary;
class Phrase;
class Memory;

struct CommitEntry : DictEntry {
  std::vector<const DictEntry*> elements;
  Memory* memory;

  CommitEntry(Memory* a_memory = NULL) : memory(a_memory) {}
  bool empty() const { return text.empty(); }
  void Clear();
  void AppendPhrase(const shared_ptr<Phrase>& phrase);
  bool Save() const;
};

class Language {
};

class Memory {
 public:
  Memory(Engine* engine, const std::string& name_space);
  virtual ~Memory();

  virtual bool Memorize(const CommitEntry& commit_entry) = 0;

  Language* language() { return &language_; }

  Dictionary* dict() const { return dict_.get(); }
  UserDictionary* user_dict() const { return user_dict_.get(); }

 protected:
  void OnCommit(Context* ctx);
  void OnDeleteEntry(Context* ctx);
  void OnUnhandledKey(Context* ctx, const KeyEvent& key);

  scoped_ptr<Dictionary> dict_;
  scoped_ptr<UserDictionary> user_dict_;

 private:
  boost::signals::connection commit_connection_;
  boost::signals::connection delete_connection_;
  boost::signals::connection unhandled_key_connection_;
  Language language_;
};

}  // namespace rime

#endif  // RIME_MEMORY_H_
