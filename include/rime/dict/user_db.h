//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_H_
#define RIME_USER_DB_H_

#include <stdint.h>
#include <string>
#include <rime/dict/db_utils.h>

namespace rime {

typedef uint64_t TickCount;

struct UserDbValue {
  int commits;
  double dee;
  TickCount tick;

  UserDbValue();
  UserDbValue(const std::string& value);
  std::string Pack() const;
  bool Unpack(const std::string &value);
};

template <class BaseDb>
class UserDb : public BaseDb {
 public:
  explicit UserDb(const std::string& name);
  virtual bool CreateMetadata();
  virtual bool Backup(const std::string& snapshot_file);
  virtual bool Restore(const std::string& snapshot_file);

  bool IsUserDb();
  std::string GetDbName();
  std::string GetUserId();
  std::string GetRimeVersion();

  static const std::string extension;
  static const std::string snapshot_extension;
};

class UserDbMerger : public Sink {
 public:
  explicit UserDbMerger(Db* db);
  virtual ~UserDbMerger();

  virtual bool MetaPut(const std::string& key, const std::string& value);
  virtual bool Put(const std::string& key, const std::string& value);

  void CloseMerge();

 protected:
  Db* db_;
  TickCount our_tick_;
  TickCount their_tick_;
  TickCount max_tick_;
  int merged_entries_;
};

class UserDbImporter : public Sink {
 public:
  explicit UserDbImporter(Db* db);

  virtual bool MetaPut(const std::string& key, const std::string& value);
  virtual bool Put(const std::string& key, const std::string& value);

 protected:
  Db* db_;
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
