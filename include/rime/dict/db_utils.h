//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-18 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DB_UTILS_H_
#define RIME_DB_UTILS_H_

#include <rime/common.h>
#include <rime/dict/tsv.h>

namespace rime {

class Db;
class DbAccessor;

class DbSink : public TsvSink {
 public:
  DbSink(Db* db);

  virtual bool MetaPut(const std::string& key, const std::string& value);
  virtual bool Put(const std::string& key, const std::string& value);

 protected:
  Db* db_;
};

class DbSource : public TsvSource {
 public:
  DbSource(Db* db);

  virtual bool MetaGet(std::string* key, std::string* value);
  virtual bool Get(std::string* key, std::string* value);

 protected:
  Db* db_;
  shared_ptr<DbAccessor> metadata_;
  shared_ptr<DbAccessor> data_;
};

}  // namespace rime

#endif  // RIME_DB_UTILS_H_
