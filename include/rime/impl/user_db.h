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

// TODO:
class UserDbAccessor {
};

class UserDb {
 public:
  UserDb(const std::string &name);
  virtual ~UserDb();

  bool Exists() const;
  bool Remove();
  bool Open();
  bool Close();

  UserDbAccessor Query(const std::string &key, bool prefix_match);
  bool Fetch(const std::string &key, std::string *value);
  bool Update(const std::string &key, const std::string &value);
  bool Erase(const std::string &key, const std::string &value);

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
