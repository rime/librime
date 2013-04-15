//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/filesystem.hpp>
#include <rime_version.h>
#include <rime/common.h>
#include <rime/service.h>
#include <rime/dict/tree_db.h>

namespace rime {

// TreeDbAccessor memebers

TreeDbAccessor::TreeDbAccessor() {
}

TreeDbAccessor::TreeDbAccessor(kyotocabinet::DB::Cursor* cursor,
                               const std::string& prefix)
    : cursor_(cursor) {
  prefix_ = prefix;
  Initialize();
}

TreeDbAccessor::~TreeDbAccessor() {
  cursor_.reset();
}

bool TreeDbAccessor::Reset() {
  return cursor_ && cursor_->jump();
}

bool TreeDbAccessor::Forward(const std::string &key) {
  return cursor_ && cursor_->jump(key);
}

bool TreeDbAccessor::Backward(const std::string &key) {
  return cursor_ && cursor_->jump_back(key);
}

bool TreeDbAccessor::GetNextRecord(std::string *key, std::string *value) {
  if (!cursor_ || !key || !value)
    return false;
  return cursor_->get(key, value, true) && MatchesPrefix(*key);
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

shared_ptr<DbAccessor> TreeDb::Query(const std::string &key) {
  if (!loaded())
    return shared_ptr<DbAccessor>();
  return boost::make_shared<TreeDbAccessor>(db_->cursor(), key);
}

bool TreeDb::Fetch(const std::string &key, std::string *value) {
  if (!value || !loaded())
    return false;
  return db_->get(key, value);
}

bool TreeDb::Update(const std::string &key, const std::string &value) {
  if (!loaded()) return false;
  DLOG(INFO) << "update db entry: " << key << " => " << value;
  return db_->set(key, value);
}

bool TreeDb::Erase(const std::string &key) {
  if (!loaded()) return false;
  DLOG(INFO) << "erase db entry: " << key;
  return db_->remove(key);
}

bool TreeDb::Backup() {
  if (!loaded()) return false;
  Deployer& deployer(Service::instance().deployer());
  boost::filesystem::path dir(deployer.user_data_sync_dir());
  if (!boost::filesystem::exists(dir)) {
    if (!boost::filesystem::create_directories(dir)) {
      LOG(ERROR) << "error creating directory '" << dir.string() << "'.";
      return false;
    }
  }
  LOG(INFO) << "backing up db '" << name_ << "' into " << dir.string();
  boost::filesystem::path snapshot_file = dir / (name_ + ".snapshot");
  bool success = db_->dump_snapshot(snapshot_file.string());
  if (!success) {
    LOG(ERROR) << "failed to create snapshot file '" << snapshot_file.string()
               << "' for  db '" << name_ << "'.";
  }
  return success;
}

bool TreeDb::Recover() {
  LOG(INFO) << "trying to recover db '" << name_ << "'.";
  // first try to open treedb with repair option on
  if (OpenReadOnly()) {
    LOG(INFO) << "treedb repaired.";
    Close();
  }
  else if (Exists()) {
    // repair doesn't work on the damanged db file; remove and recreate it
    LOG(INFO) << "recreating db file.";
    boost::system::error_code ec;
    boost::filesystem::rename(file_name(), file_name() + ".old", ec);
    if (ec && !Remove()) {
      LOG(ERROR) << "Error removing db file '" << file_name() << "'.";
      return false;
    }
  }
  if (Open()) {
    Deployer& deployer(Service::instance().deployer());
    boost::filesystem::path snapshot_file(deployer.user_data_sync_dir());
    snapshot_file /= (name_ + ".snapshot");
    if (boost::filesystem::exists(snapshot_file)) {
      LOG(INFO) << "snapshot exists, trying to recover db '" << name_ << "'.";
      if (Restore(snapshot_file.string())) {
        LOG(INFO) << "recovered db '" << name_ << "' from snapshot.";
      }
    }
    LOG(INFO) << "recovery successful.";
    return true;
  }
  LOG(ERROR) << "recovery failed.";
  return false;
}

bool TreeDb::Restore(const std::string& snapshot_file) {
  if (!loaded()) return false;
  bool success = db_->load_snapshot(snapshot_file);
  if (!success) {
    LOG(ERROR) << "failed to restore db from '" << snapshot_file << "'.";
  }
  return success;
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
    if (!Fetch("\x01/db_name", &db_name)) {
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
  LOG(INFO) << "creating metadata for db '" << name_ << "'.";
  Deployer& deployer(Service::instance().deployer());
  // '\x01' is the meta character
  return db_->set("\x01/db_name", name_) &&
      db_->set("\x01/db_type", db_type_) &&
      db_->set("\x01/rime_version", RIME_VERSION) &&
      db_->set("\x01/user_id", deployer.user_id);
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
