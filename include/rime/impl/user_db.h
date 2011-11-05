// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// A simple wrapper for kyotocabinet::TreeDB
// 
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_H_
#define RIME_USER_DB_H_

#include <string>
#include <kchashdb.h>
#include <rime/common.h>

namespace rime {

class UserDbAccessor {
 public:
  UserDbAccessor() : key_(), cursor_(NULL) {}
  explicit UserDbAccessor(const std::string &key,
                          kyotocabinet::DB::Cursor *cursor)
      : key_(key), cursor_(cursor) {}
  ~UserDbAccessor() {
    if (cursor_) {
      delete cursor_;
      cursor_ = NULL;
    }
  }
  bool Yield(std::string *key, std::string *value);
  bool exhausted();

 private:
  std::string key_;
  kyotocabinet::DB::Cursor *cursor_;
};

class UserDb {
 public:
  UserDb(const std::string &name);
  virtual ~UserDb();

  bool Exists() const;
  bool Remove();
  bool Open();
  bool Close();

  const UserDbAccessor Query(const std::string &key);
  bool Fetch(const std::string &key, std::string *value);
  bool Update(const std::string &key, const std::string &value);
  bool Erase(const std::string &key);

  const std::string& name() const { return name_; }
  const std::string& file_name() const { return file_name_; }
  bool loaded() const { return loaded_; }

 protected:
  bool CreateMetadata();
  
  std::string name_;
  std::string file_name_;
  bool loaded_;
  scoped_ptr<kyotocabinet::TreeDB> db_;
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
