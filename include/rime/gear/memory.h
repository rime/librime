//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-01-02 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MEMORY_H_
#define RIME_MEMORY_H_

#include <vector>
#include <boost/signals/connection.hpp>
#include <rime/common.h>

namespace rime {

class KeyEvent;
class Context;
class Engine;
class Dictionary;
class UserDictionary;
struct DictEntry;

class Language {
};

class Memory : public Language {
 public:
  Memory(Engine* engine);
  virtual ~Memory();
  
  virtual bool Memorize(const DictEntry& commit_entry,
                        const std::vector<const DictEntry*>& elements) = 0;

  // TODO
  Language* language() { return this; }
  
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
};

}  // namespace rime

#endif  // RIME_MEMORY_H_
