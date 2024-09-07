//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2024-09-07 WhiredPlanck <whiredplanck@outlook.com>
//
#ifndef RIME_ROCKS_DB_H_
#define RIME_ROCKS_DB_H_

#include <rime/dict/db.h>

namespace rime {

struct RocksDbCursor;
struct RocksDbWrapper;

class RocksDb;

class RocksDbAccessor : public DbAccessor {
 public:
  RocksDbAccessor();
  RocksDbAccessor(RocksDbCursor* cursor, const string& prefix);
  virtual ~RocksDbAccessor();

  bool Reset() override;
  bool Jump(const string& key) override;
  bool GetNextRecord(string* key, string* value) override;
  bool exhausted() override;

 private:
  the<RocksDbCursor> cursor_;
  bool is_metadata_query_ = false;
};

class RocksDb : public Db, public Recoverable, public Transactional {
 public:
  RocksDb(const path& file_path,
          const string& db_name,
          const string& db_type = "");
  virtual ~RocksDb();

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

  the<RocksDbWrapper> db_;
  string db_type_;
};

}  // namespace rime

#endif  // RIME_ROCKS_DB_H_
