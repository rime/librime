//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/filesystem.hpp>
#include <rime/common.h>
#include <rime/service.h>
#include <rime/dict/tree_db.h>

namespace rime {

static const char* kMetaCharacter = "\x01";

// TreeDbAccessor memebers

TreeDbAccessor::TreeDbAccessor() {
}

TreeDbAccessor::TreeDbAccessor(kyotocabinet::DB::Cursor* cursor,
                               const std::string& prefix)
    : DbAccessor(prefix), cursor_(cursor) {
  Reset();
}

TreeDbAccessor::~TreeDbAccessor() {
  cursor_.reset();
}

bool TreeDbAccessor::Reset() {
  return cursor_ && cursor_->jump(prefix_);
}

bool TreeDbAccessor::Jump(const std::string &key) {
  return cursor_ && cursor_->jump(key);
}

bool TreeDbAccessor::GetNextRecord(std::string *key, std::string *value) {
  if (!cursor_ || !key || !value)
    return false;
  bool got = cursor_->get(key, value, true) && MatchesPrefix(*key);
  if (got && prefix_ == kMetaCharacter) {
    key->erase(0, 1);  // remove meta character
  }
  return got;
}

bool TreeDbAccessor::exhausted() {
  std::string key;
  return !cursor_->get_key(&key, false) || !MatchesPrefix(key);
}

// TreeDb members

TreeDb::TreeDb(const std::string& name, const std::string& db_type)
    : Db(name), db_type_(db_type) {
}

TreeDb::~TreeDb() {
  if (loaded())
    Close();
}

void TreeDb::Initialize() {
  db_.reset(new kyotocabinet::TreeDB);
  db_->tune_options(kyotocabinet::TreeDB::TSMALL |
                    kyotocabinet::TreeDB::TLINEAR);
  db_->tune_map(4LL << 20);
  db_->tune_defrag(8);
}

shared_ptr<DbAccessor> TreeDb::QueryMetadata() {
  return Query(kMetaCharacter);
}

shared_ptr<DbAccessor> TreeDb::QueryAll() {
  shared_ptr<DbAccessor> all = Query("");
  if (all)
    all->Jump(" ");  // skip metadata
  return all;
}

shared_ptr<DbAccessor> TreeDb::Query(const std::string &key) {
  if (!loaded())
    return shared_ptr<DbAccessor>();
  return make_shared<TreeDbAccessor>(db_->cursor(), key);
}

bool TreeDb::Fetch(const std::string &key, std::string *value) {
  if (!value || !loaded())
    return false;
  return db_->get(key, value);
}

bool TreeDb::Update(const std::string &key, const std::string &value) {
  if (!loaded() || readonly()) return false;
  DLOG(INFO) << "update db entry: " << key << " => " << value;
  return db_->set(key, value);
}

bool TreeDb::Erase(const std::string &key) {
  if (!loaded() || readonly()) return false;
  DLOG(INFO) << "erase db entry: " << key;
  return db_->remove(key);
}

bool TreeDb::Backup(const std::string& snapshot_file) {
  if (!loaded()) return false;
  LOG(INFO) << "backing up db '" << name() << "' to " << snapshot_file;
  bool success = db_->dump_snapshot(snapshot_file);
  if (!success) {
    LOG(ERROR) << "failed to create snapshot file '" << snapshot_file
               << "' for db '" << name() << "'.";
  }
  return success;
}

bool TreeDb::Restore(const std::string& snapshot_file) {
  if (!loaded() || readonly()) return false;
  bool success = db_->load_snapshot(snapshot_file);
  if (!success) {
    LOG(ERROR) << "failed to restore db '" << name()
               << "' from '" << snapshot_file << "'.";
  }
  return success;
}

bool TreeDb::Recover() {
  LOG(INFO) << "trying to recover db '" << name() << "'.";
  // first try to open the db with repair option on
  if (OpenReadOnly()) {
    LOG(INFO) << "repair finished.";
    if (Close() && Open()) {
      LOG(INFO) << "treedb recovery successful.";
      return true;
    }
  }
  LOG(ERROR) << "treedb recovery failed.";
  return false;
}

bool TreeDb::Open() {
  if (loaded()) return false;
  Initialize();
  readonly_ = false;
  loaded_ = db_->open(file_name(),
                      kyotocabinet::TreeDB::OWRITER |
                      kyotocabinet::TreeDB::OCREATE |
                      kyotocabinet::TreeDB::OTRYLOCK |
                      kyotocabinet::TreeDB::ONOREPAIR);
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
    LOG(ERROR) << "Error opening db '" << name_ << "'.";
  }
  return loaded_;
}

bool TreeDb::OpenReadOnly() {
  if (loaded()) return false;
  Initialize();
  readonly_ = true;
  loaded_ = db_->open(file_name(),
                      kyotocabinet::TreeDB::OREADER |
                      kyotocabinet::TreeDB::OTRYLOCK);
  if (!loaded_) {
    LOG(ERROR) << "Error opening db '" << name_ << "' read-only.";
  }
  return loaded_;
}

bool TreeDb::Close() {
  if (!loaded()) return false;
  db_->close();
  LOG(INFO) << "closed db '" << name_ << "'.";
  loaded_ = false;
  readonly_ = false;
  in_transaction_ = false;
  return true;
}

bool TreeDb::CreateMetadata() {
  return Db::CreateMetadata() &&
      MetaUpdate("/db_type", db_type_);
}

bool TreeDb::MetaFetch(const std::string &key, std::string *value) {
  return Fetch(kMetaCharacter + key, value);
}

bool TreeDb::MetaUpdate(const std::string &key, const std::string &value) {
  return Update(kMetaCharacter + key, value);
}

bool TreeDb::BeginTransaction() {
  if (!loaded()) return false;
  in_transaction_ = db_->begin_transaction();
  return in_transaction_;
}

bool TreeDb::AbortTransaction() {
  if (!loaded() || !in_transaction()) return false;
  in_transaction_ = !db_->end_transaction(false);
  return !in_transaction_;
}

bool TreeDb::CommitTransaction() {
  if (!loaded() || !in_transaction()) return false;
  in_transaction_ = !db_->end_transaction(true);
  return !in_transaction_;
}

}  // namespace rime
