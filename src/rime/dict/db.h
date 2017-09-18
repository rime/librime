//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DB_H_
#define RIME_DB_H_

#include <rime_api.h>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

class DbAccessor {
 public:
  DbAccessor() = default;
  explicit DbAccessor(const string& prefix)
      : prefix_(prefix) {}
  virtual ~DbAccessor() = default;

  virtual bool Reset() = 0;
  virtual bool Jump(const string &key) = 0;
  virtual bool GetNextRecord(string *key, string *value) = 0;
  virtual bool exhausted() = 0;

 protected:
  bool MatchesPrefix(const string& key);

  string prefix_;
};

class Db : public Class<Db, const string&> {
 public:
  explicit Db(const string& name);
  virtual ~Db() = default;

  RIME_API bool Exists() const;
  RIME_API virtual bool Remove();

  virtual bool Open() = 0;
  virtual bool OpenReadOnly() = 0;
  virtual bool Close() = 0;

  virtual bool Backup(const string& snapshot_file) = 0;
  virtual bool Restore(const string& snapshot_file) = 0;

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const string& key, string* value) = 0;
  virtual bool MetaUpdate(const string& key, const string& value) = 0;

  virtual an<DbAccessor> QueryMetadata() = 0;
  virtual an<DbAccessor> QueryAll() = 0;
  virtual an<DbAccessor> Query(const string &key) = 0;
  virtual bool Fetch(const string &key, string *value) = 0;
  virtual bool Update(const string &key, const string &value) = 0;
  virtual bool Erase(const string &key) = 0;

  const string& name() const { return name_; }
  const string& file_name() const { return file_name_; }
  bool loaded() const { return loaded_; }
  bool readonly() const { return readonly_; }
  bool disabled() const { return disabled_; }
  void disable() { disabled_ = true; }
  void enable() { disabled_ = false; }

 protected:
  string name_;
  string file_name_;
  bool loaded_ = false;
  bool readonly_ = false;
  bool disabled_ = false;
};

class Transactional {
 public:
  Transactional() = default;
  virtual ~Transactional() = default;
  virtual bool BeginTransaction() { return false; }
  virtual bool AbortTransaction() { return false; }
  virtual bool CommitTransaction() { return false; }
  bool in_transaction() const { return in_transaction_; }
 protected:
  bool in_transaction_ = false;
};

class Recoverable {
 public:
  virtual ~Recoverable() = default;
  virtual bool Recover() = 0;
};

}  // namespace rime

#endif  // RIME_DB_H_
