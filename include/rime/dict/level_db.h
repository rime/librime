//
// Copyleft RIME Developers
// License: GPLv3
//
// 2014-12-04 Chen Gong <chen.sst@gmail.com>
//
#ifndef RIME_LEVEL_DB_H_
#define RIME_LEVEL_DB_H_

#include <string>
#include <rime/dict/db.h>

namespace rime {

struct LevelDbCursor;
struct LevelDbWrapper;

class LevelDb;

class LevelDbAccessor : public DbAccessor {
 public:
  LevelDbAccessor();
  LevelDbAccessor(LevelDbCursor* cursor,
                 const std::string& prefix);
  virtual ~LevelDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const std::string& key);
  virtual bool GetNextRecord(std::string* key, std::string* value);
  virtual bool exhausted();

 private:
  unique_ptr<LevelDbCursor> cursor_;
  bool is_metadata_query_ = false;
};

class LevelDb : public Db,
                public Recoverable,
                public Transactional {
 public:
  LevelDb(const std::string& name, const std::string& db_type = "");
  virtual ~LevelDb();

  virtual bool Remove();
  virtual bool Open();
  virtual bool OpenReadOnly();
  virtual bool Close();

  virtual bool Backup(const std::string& snapshot_file);
  virtual bool Restore(const std::string& snapshot_file);

  virtual bool CreateMetadata();
  virtual bool MetaFetch(const std::string& key, std::string* value);
  virtual bool MetaUpdate(const std::string& key, const std::string& value);

  virtual shared_ptr<DbAccessor> QueryMetadata();
  virtual shared_ptr<DbAccessor> QueryAll();
  virtual shared_ptr<DbAccessor> Query(const std::string& key);
  virtual bool Fetch(const std::string& key, std::string* value);
  virtual bool Update(const std::string& key, const std::string& value);
  virtual bool Erase(const std::string& key);

  // Recoverable
  virtual bool Recover();

  // Transactional
  virtual bool BeginTransaction();
  virtual bool AbortTransaction();
  virtual bool CommitTransaction();

 private:
  void Initialize();

  unique_ptr<LevelDbWrapper> db_;
  std::string db_type_;
};

}  // namespace rime

#endif  // RIME_LEVEL_DB_H_
