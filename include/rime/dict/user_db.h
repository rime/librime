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

#if defined(_MSC_VER)
#pragma warning(disable: 4244)
#pragma warning(disable: 4351)
#endif
#include <kchashdb.h>
#if defined(_MSC_VER)
#pragma warning(default: 4351)
#pragma warning(default: 4244)
#endif

#include <string>
#include <rime/common.h>

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

class Db {
 public:
  explicit Db(const std::string &name);
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

// transaction support
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

// TreeDb

class TreeDb;

class TreeDbAccessor : public DbAccessor {
 public:
  TreeDbAccessor();
  TreeDbAccessor(kyotocabinet::DB::Cursor *cursor,
                 const std::string &prefix);
  virtual ~TreeDbAccessor();

  virtual bool Reset();
  virtual bool Forward(const std::string &key);
  virtual bool Backward(const std::string &key);
  virtual bool GetNextRecord(std::string *key, std::string *value);
  virtual bool exhausted();

 private:
  scoped_ptr<kyotocabinet::DB::Cursor> cursor_;
};

class TreeDb : public Db,
               public Managed,
               public Transactional {
 public:
  TreeDb(const std::string& name, const std::string& db_type = "");
  virtual ~TreeDb();

  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();
  virtual bool CreateMetadata();

  virtual shared_ptr<DbAccessor> Query(const std::string &key);
  virtual bool Fetch(const std::string &key, std::string *value);
  virtual bool Update(const std::string &key, const std::string &value);
  virtual bool Erase(const std::string &key);

  // Managed
  virtual bool Recover();
  virtual bool Backup();
  virtual bool Restore(const std::string& snapshot_file);

  // Transactional
  virtual bool BeginTransaction();
  virtual bool AbortTransaction();
  virtual bool CommitTransaction();

 private:
  void Initialize();

  scoped_ptr<kyotocabinet::TreeDB> db_;
  std::string db_type_;
};

typedef uint64_t TickCount;

class UserDb : public TreeDb {
 public:
  UserDb(const std::string& name)
      : TreeDb(name + ".userdb.kct", "userdb") {
  }
  bool IsUserDb();
  const std::string GetDbName();
  const std::string GetUserId();
  TickCount GetTickCount();
};

}  // namespace rime

#endif  // RIME_USER_DB_H_
