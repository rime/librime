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
  LevelDbAccessor(LevelDbCursor* cursor, const string& prefix);
  virtual ~LevelDbAccessor();

  bool Reset() override;
  bool Jump(const string& key) override;
  bool GetNextRecord(string* key, string* value) override;
  bool exhausted() override;

 private:
  the<LevelDbCursor> cursor_;
  bool is_metadata_query_ = false;
};

class LevelDb : public Db, public Recoverable, public Transactional {
 public:
  LevelDb(const path& file_path,
          const string& db_name,
          const string& db_type = "");
  virtual ~LevelDb();

  bool Remove() override;
  bool Open() override;
  bool OpenReadOnly() override;
  bool Close() override;

  bool Backup(const path& snapshot_file) override;
  bool Restore(const path& snapshot_file) override;

  bool CreateMetadata() override;
  bool MetaFetch(const string& key, string* value) override;
  bool MetaUpdate(const string& key, const string& value) override;

  an<DbAccessor> QueryMetadata() override;
  an<DbAccessor> QueryAll() override;
  an<DbAccessor> Query(const string& key) override;
  bool Fetch(const string& key, string* value) override;
  bool Update(const string& key, const string& value) override;
  bool Erase(const string& key) override;

  // Recoverable
  bool Recover() override;

  // Transactional
  bool BeginTransaction() override;
  bool AbortTransaction() override;
  bool CommitTransaction() override;

 private:
  void Initialize();

  the<LevelDbWrapper> db_;
  string db_type_;
};

}  // namespace rime

#endif  // RIME_LEVEL_DB_H_
