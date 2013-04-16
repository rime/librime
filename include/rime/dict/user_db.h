//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_H_
#define RIME_USER_DB_H_

#include <string>
#include <rime/dict/tree_db.h>

namespace rime {

typedef uint64_t TickCount;

class UserDb : public TreeDb {
 public:
  UserDb(const std::string& name)
      : TreeDb(name + ".userdb.kct", "userdb") {
  }
  virtual bool CreateMetadata();
  bool IsUserDb();
  const std::string GetDbName();
  const std::string GetUserId();
  const std::string GetRimeVersion();
  TickCount GetTickCount();
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
