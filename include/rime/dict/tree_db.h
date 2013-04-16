//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// A simple wrapper for kyotocabinet::TreeDB
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_TREE_DB_H_
#define RIME_TREE_DB_H_

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
#include <rime/dict/db.h>

namespace rime {

class TreeDb;

class TreeDbAccessor : public DbAccessor {
 public:
  TreeDbAccessor();
  TreeDbAccessor(kyotocabinet::DB::Cursor *cursor,
                 const std::string &prefix);
  virtual ~TreeDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const std::string &key);
  virtual bool GetNextRecord(std::string *key, std::string *value);
  virtual bool exhausted();

 private:
  scoped_ptr<kyotocabinet::DB::Cursor> cursor_;
};

class TreeDb : public Db,
               public Recoverable,
               public Transactional {
 public:
  TreeDb(const std::string& name, const std::string& db_type = "");
  virtual ~TreeDb();

  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();

  virtual bool Backup(const std::string& snapshot_file);
  virtual bool Restore(const std::string& snapshot_file);

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const std::string &key, std::string *value);
  virtual bool MetaUpdate(const std::string &key, const std::string &value);

  virtual shared_ptr<DbAccessor> Query(const std::string &key);
  virtual bool Fetch(const std::string &key, std::string *value);
  virtual bool Update(const std::string &key, const std::string &value);
  virtual bool Erase(const std::string &key);

  // Recoverable
  virtual bool Recover();

  // Transactional
  virtual bool BeginTransaction();
  virtual bool AbortTransaction();
  virtual bool CommitTransaction();

 private:
  void Initialize();

  scoped_ptr<kyotocabinet::TreeDB> db_;
  std::string db_type_;
};

}  // namespace rime

#endif  // RIME_TREE_DB_H_
