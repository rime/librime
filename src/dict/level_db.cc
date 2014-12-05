//
// Copyleft RIME Developers
// License: GPLv3
//
// 2014-12-04 Chen Gong <chen.sst@gmail.com>
//

#include <boost/filesystem.hpp>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <rime/common.h>
#include <rime/service.h>
#include <rime/dict/level_db.h>
#include <rime/dict/user_db.h>

namespace rime {

static const char* kMetaCharacter = "\x01";

struct LevelDbCursor {
  leveldb::Iterator* iterator = nullptr;

  LevelDbCursor(leveldb::DB* db) {
    leveldb::ReadOptions options;
    options.fill_cache = false;
    iterator = db->NewIterator(options);
  }

  bool IsValid() const {
    return iterator && iterator->Valid();
  }

  std::string GetKey() const {
    return iterator->key().ToString();
  }

  std::string GetValue() const {
    return iterator->value().ToString();
  }

  void Next() {
    iterator->Next();
  }

  bool Jump(const std::string& key) {
    if (!iterator) {
      return false;
    }
    iterator->Seek(key);
    return true;
  }

  void Release() {
    delete iterator;
    iterator = nullptr;
  }
};

struct LevelDbWrapper {
  leveldb::DB* ptr = nullptr;
  leveldb::WriteBatch batch;

  leveldb::Status Open(const std::string& file_name, bool readonly) {
    leveldb::Options options;
    options.create_if_missing = !readonly;
    return leveldb::DB::Open(options, file_name, &ptr);
  }

  void Release() {
    delete ptr;
    ptr = nullptr;
  }

  LevelDbCursor* CreateCursor() {
    return new LevelDbCursor(ptr);
  }

  bool Fetch(const std::string& key, std::string* value) {
    auto status = ptr->Get(leveldb::ReadOptions(), key, value);
    return status.ok();
  }

  bool Update(const std::string& key, const std::string& value, bool write_batch) {
    if (write_batch) {
      batch.Put(key, value);
      return true;
    }
    auto status = ptr->Put(leveldb::WriteOptions(), key, value);
    return status.ok();
  }

  bool Erase(const std::string& key, bool write_batch) {
    if (write_batch) {
      batch.Delete(key);
      return true;
    }
    auto status = ptr->Delete(leveldb::WriteOptions(), key);
    return status.ok();
  }

  void ClearBatch() {
    batch.Clear();
  }

  bool CommitBatch() {
    auto status = ptr->Write(leveldb::WriteOptions(), &batch);
    return status.ok();
  }

};

// LevelDbAccessor memebers

LevelDbAccessor::LevelDbAccessor() {
}

LevelDbAccessor::LevelDbAccessor(LevelDbCursor* cursor,
                                 const std::string& prefix)
    : DbAccessor(prefix), cursor_(cursor),
      is_metadata_query_(prefix == kMetaCharacter) {
  Reset();
}

LevelDbAccessor::~LevelDbAccessor() {
  cursor_->Release();
}

bool LevelDbAccessor::Reset() {
  return cursor_->Jump(prefix_);
}

bool LevelDbAccessor::Jump(const std::string& key) {
  return cursor_->Jump(key);
}

bool LevelDbAccessor::GetNextRecord(std::string* key, std::string* value) {
  if (!cursor_->IsValid() || !key || !value)
    return false;
  *key = cursor_->GetKey();
  if (!MatchesPrefix(*key)) {
    return false;
  }
  if (is_metadata_query_) {
    key->erase(0, 1);  // remove meta character
  }
  *value = cursor_->GetValue();
  cursor_->Next();
  return true;
}

bool LevelDbAccessor::exhausted() {
  return !cursor_->IsValid() || !MatchesPrefix(cursor_->GetKey());
}

// LevelDb members

LevelDb::LevelDb(const std::string& name, const std::string& db_type)
    : Db(name), db_type_(db_type) {
}

LevelDb::~LevelDb() {
  if (loaded())
    Close();
}

void LevelDb::Initialize() {
  db_.reset(new LevelDbWrapper);
}

shared_ptr<DbAccessor> LevelDb::QueryMetadata() {
  return Query(kMetaCharacter);
}

shared_ptr<DbAccessor> LevelDb::QueryAll() {
  shared_ptr<DbAccessor> all = Query("");
  if (all)
    all->Jump(" ");  // skip metadata
  return all;
}

shared_ptr<DbAccessor> LevelDb::Query(const std::string& key) {
  if (!loaded())
    return nullptr;
  return New<LevelDbAccessor>(db_->CreateCursor(), key);
}

bool LevelDb::Fetch(const std::string& key, std::string* value) {
  if (!value || !loaded())
    return false;
  return db_->Fetch(key, value);
}

bool LevelDb::Update(const std::string& key, const std::string& value) {
  if (!loaded() || readonly())
    return false;
  DLOG(INFO) << "update db entry: " << key << " => " << value;
  return db_->Update(key, value, in_transaction());
}

bool LevelDb::Erase(const std::string& key) {
  if (!loaded() || readonly())
    return false;
  DLOG(INFO) << "erase db entry: " << key;
  return db_->Erase(key, in_transaction());
}

bool LevelDb::Backup(const std::string& snapshot_file) {
  if (!loaded())
    return false;
  LOG(INFO) << "backing up db '" << name() << "' to " << snapshot_file;
  // TODO(chen): suppose we only use this method for user dbs.
  bool success = UserDbHelper(this).UniformBackup(snapshot_file);
  if (!success) {
    LOG(ERROR) << "failed to create snapshot file '" << snapshot_file
               << "' for db '" << name() << "'.";
  }
  return success;
}

bool LevelDb::Restore(const std::string& snapshot_file) {
  if (!loaded() || readonly())
    return false;
  // TODO(chen): suppose we only use this method for user dbs.
  bool success = UserDbHelper(this).UniformRestore(snapshot_file);
  if (!success) {
    LOG(ERROR) << "failed to restore db '" << name()
               << "' from '" << snapshot_file << "'.";
  }
  return success;
}

bool LevelDb::Recover() {
  LOG(INFO) << "trying to recover db '" << name() << "'.";
  auto status = leveldb::RepairDB(file_name(), leveldb::Options());
  if (status.ok()) {
    LOG(INFO) << "repair finished.";
    if (Close() && Open()) {
      LOG(INFO) << "db recovery successful.";
      return true;
    }
  }
  LOG(ERROR) << "db recovery failed: " << status.ToString();
  return false;
}

bool LevelDb::Remove() {
  if (loaded()) {
    LOG(ERROR) << "attempt to remove opened db '" << name_ << "'.";
    return false;
  }
  auto status = leveldb::DestroyDB(file_name(), leveldb::Options());
  if (!status.ok()) {
    LOG(ERROR) << "Error removing db '" << name_ << "': " << status.ToString();
    return false;
  }
  return true;
}

bool LevelDb::Open() {
  if (loaded())
    return false;
  Initialize();
  readonly_ = false;
  auto status = db_->Open(file_name(), readonly_);
  loaded_ = status.ok();

  if (loaded_) {
    std::string db_name;
    if (!MetaFetch("/db_name", &db_name)) {
      if (!CreateMetadata()) {
        LOG(ERROR) << "error creating metadata.";
        Close();
      }
    }
  }
  else {
    LOG(ERROR) << "Error opening db '" << name_ << "': " << status.ToString();
  }
  return loaded_;
}

bool LevelDb::OpenReadOnly() {
  if (loaded())
    return false;
  Initialize();
  readonly_ = true;
  auto status = db_->Open(file_name(), readonly_);
  loaded_ = status.ok();

  if (!loaded_) {
    LOG(ERROR) << "Error opening db '" << name_ << "' read-only.";
  }
  return loaded_;
}

bool LevelDb::Close() {
  if (!loaded())
    return false;

  db_->Release();

  LOG(INFO) << "closed db '" << name_ << "'.";
  loaded_ = false;
  readonly_ = false;
  in_transaction_ = false;
  return true;
}

bool LevelDb::CreateMetadata() {
  return Db::CreateMetadata() &&
      MetaUpdate("/db_type", db_type_);
}

bool LevelDb::MetaFetch(const std::string& key, std::string* value) {
  return Fetch(kMetaCharacter + key, value);
}

bool LevelDb::MetaUpdate(const std::string& key, const std::string& value) {
  return Update(kMetaCharacter + key, value);
}

bool LevelDb::BeginTransaction() {
  if (!loaded())
    return false;
  db_->ClearBatch();
  in_transaction_ = true;
  return true;
}

bool LevelDb::AbortTransaction() {
  if (!loaded() || !in_transaction())
    return false;
  db_->ClearBatch();
  in_transaction_ = false;
  return true;
}

bool LevelDb::CommitTransaction() {
  if (!loaded() || !in_transaction())
    return false;
  bool ok = db_->CommitBatch();
  db_->ClearBatch();
  in_transaction_ = false;
  return ok;
}

template <>
const std::string UserDbFormat<LevelDb>::extension(".userdb");

template <>
const std::string UserDbFormat<LevelDb>::snapshot_extension(".userdb.txt");

template <>
UserDbWrapper<LevelDb>::UserDbWrapper(const std::string& db_name)
    : LevelDb(db_name, "userdb") {
}

}  // namespace rime
