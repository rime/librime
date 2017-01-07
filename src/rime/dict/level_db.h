//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-12-04 Chen Gong <chen.sst@gmail.com>
//
#ifndef RIME_LEVEL_DB_H_
#define RIME_LEVEL_DB_H_

#include <rime/dict/db.h>

namespace rime {

struct LevelDbCursor;
struct LevelDbWrapper;

class LevelDb;

class LevelDbAccessor : public DbAccessor {
 public:
  LevelDbAccessor();
  LevelDbAccessor(LevelDbCursor* cursor,
                 const string& prefix);
  virtual ~LevelDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const string& key);
  virtual bool GetNextRecord(string* key, string* value);
  virtual bool exhausted();

 private:
  the<LevelDbCursor> cursor_;
  bool is_metadata_query_ = false;
};

class LevelDb : public Db,
                public Recoverable,
                public Transactional {
 public:
  LevelDb(const string& name, const string& db_type = "");
  virtual ~LevelDb();

  virtual bool Remove();
  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();

  virtual bool Backup(const string& snapshot_file);
  virtual bool Restore(const string& snapshot_file);

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const string& key, string* value);
  virtual bool MetaUpdate(const string& key, const string& value);

  virtual an<DbAccessor> QueryMetadata();
  virtual an<DbAccessor> QueryAll();
  virtual an<DbAccessor> Query(const string& key);
  virtual bool Fetch(const string& key, string* value);
  virtual bool Update(const string& key, const string& value);
  virtual bool Erase(const string& key);

  // Recoverable
  virtual bool Recover();

  // Transactional
  virtual bool BeginTransaction();
  virtual bool AbortTransaction();
  virtual bool CommitTransaction();

 private:
  void Initialize();

  the<LevelDbWrapper> db_;
  string db_type_;
};

}  // namespace rime

#endif  // RIME_LEVEL_DB_H_
