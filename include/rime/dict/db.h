//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-04-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DB_H_
#define RIME_DB_H_

#include <string>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

class DbAccessor {
 public:
  DbAccessor();
  virtual ~DbAccessor();

  virtual bool Reset() = 0;
  virtual bool Forward(const std::string &key) = 0;
  virtual bool Backward(const std::string &key) = 0;
  virtual bool GetNextRecord(std::string *key, std::string *value) = 0;
  virtual bool exhausted() = 0;

 protected:
  void Initialize();
  bool MatchesPrefix(const std::string& key);

  std::string prefix_;
};

class Db : public Class<Db, const std::string&> {
 public:
  explicit Db(const std::string& name);
  virtual ~Db();

  bool Exists() const;
  bool Remove();

  virtual bool Open() = 0;
  virtual bool OpenReadOnly() = 0;
  virtual bool Close() = 0;
  virtual bool CreateMetadata() = 0;

  virtual shared_ptr<DbAccessor> Query(const std::string &key) = 0;
  virtual bool Fetch(const std::string &key, std::string *value) = 0;
  virtual bool Update(const std::string &key, const std::string &value) = 0;
  virtual bool Erase(const std::string &key) = 0;

  const std::string& name() const { return name_; }
  const std::string& file_name() const { return file_name_; }
  bool loaded() const { return loaded_; }
  bool readonly() const { return readonly_; }
  bool disabled() const { return disabled_; }
  void disable() { disabled_ = true; }
  void enable() { disabled_ = false; }

 protected:
  std::string name_;
  std::string file_name_;
  bool loaded_;
  bool readonly_;
  bool disabled_;
};

class Transactional {
 public:
  Transactional() : in_transaction_(false) {}
  virtual ~Transactional() {}
  virtual bool BeginTransaction() { return false; }
  virtual bool AbortTransaction() { return false; }
  virtual bool CommitTransaction() { return false; }
  bool in_transaction() const { return in_transaction_; }
 protected:
  bool in_transaction_;
};

class Managed {
 public:
  virtual ~Managed() {}
  virtual bool Recover() = 0;
  virtual bool Backup() = 0;
  virtual bool Restore(const std::string& snapshot_file) = 0;
};

}  // namespace rime

#endif  // RIME_DB_H_
