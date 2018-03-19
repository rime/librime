//
// Copyleft RIME Developers
// License: GPLv3
//
// A simple wrapper around kyotocabinet::TreeDB.
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
// 2014-12-10 GONG Chen <chen.sst@gmail.com>  move to namespace rime::legacy.
//
#ifndef RIME_LEGACY_TREE_DB_H_
#define RIME_LEGACY_TREE_DB_H_

#include <rime/common.h>
#include <rime/dict/db.h>

namespace rime {
namespace legacy {

struct TreeDbCursor;
struct TreeDbWrapper;

class TreeDb;

class TreeDbAccessor : public DbAccessor {
 public:
  TreeDbAccessor();
  TreeDbAccessor(TreeDbCursor* cursor,
                 const std::string& prefix);
  virtual ~TreeDbAccessor();

  virtual bool Reset();
  virtual bool Jump(const std::string& key);
  virtual bool GetNextRecord(std::string* key, std::string* value);
  virtual bool exhausted();

 private:
  std::unique_ptr<TreeDbCursor> cursor_;
  bool is_metadata_query_ = false;
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
  virtual bool MetaFetch(const std::string& key, std::string* value);
  virtual bool MetaUpdate(const std::string& key, const std::string& value);

  virtual std::shared_ptr<DbAccessor> QueryMetadata();
  virtual std::shared_ptr<DbAccessor> QueryAll();
  virtual std::shared_ptr<DbAccessor> Query(const std::string& key);
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

  std::unique_ptr<TreeDbWrapper> db_;
  std::string db_type_;
};

}  // namespace legacy
}  // namespace rime

#endif  // RIME_LEGACY_TREE_DB_H_
