//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_H_
#define RIME_USER_DB_H_

#include <stdint.h>
#include <string>

namespace rime {

typedef uint64_t TickCount;

struct UserDbValue {
  int commits;
  double dee;
  TickCount tick;

  UserDbValue();
  UserDbValue(const std::string& value);
  const std::string Pack() const;
  bool Unpack(const std::string &value);
};

template <class BaseDb>
class UserDb : public BaseDb {
 public:
  explicit UserDb(const std::string& name);
  virtual bool CreateMetadata();

  bool IsUserDb();
  const std::string GetDbName();
  const std::string GetUserId();
  const std::string GetRimeVersion();
  TickCount GetTickCount();

  static const std::string extension;
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
