//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <rime/deployer.h>
#include <rime/dict/db.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_db.h>
#include <rime/lever/userdb_recovery_task.h>

namespace fs = boost::filesystem;

namespace rime {

UserDbRecoveryTask::UserDbRecoveryTask(shared_ptr<Db> db) : db_(db) {
  if (db_) {
    db_->disable();
  }
}

bool UserDbRecoveryTask::Run(Deployer* deployer) {
  if (!db_) {
    return false;
  }
  BOOST_SCOPE_EXIT( (&db_) ) {
    db_->enable();
  }
  BOOST_SCOPE_EXIT_END
  //
  auto r = As<Recoverable>(db_);
  if (r && r->Recover()) {
    return true;
  }
  if (db_->loaded()) {
    LOG(WARNING) << "cannot recover loaded db '" << db_->name() << "'.";
    return false;
  }
  // repair didn't work on the damanged db file; remove and recreate it
  LOG(INFO) << "recreating db file.";
  if (db_->Exists()) {
    boost::system::error_code ec;
    boost::filesystem::rename(db_->file_name(), db_->file_name() + ".old", ec);
    if (ec && !db_->Remove()) {
      LOG(ERROR) << "Error removing db file '" << db_->file_name() << "'.";
      return false;
    }
  }
  if (!db_->Open()) {
    LOG(ERROR) << "Error creating db '" << db_->name() << "'.";
    return false;
  }
  RestoreUserDataFromSnapshot(deployer);
  LOG(INFO) << "recovery successful.";
  return true;
}

void UserDbRecoveryTask::RestoreUserDataFromSnapshot(Deployer* deployer) {
  if (!Is<UserDb<TreeDb>>(db_))
    return;
  std::string dict_name(db_->name());
  boost::erase_last(dict_name, UserDb<TreeDb>::extension);
  // locate snapshot file
  boost::filesystem::path dir(deployer->user_data_sync_dir());
  // try *.userdb.txt
  fs::path snapshot_path =
      dir / (dict_name + UserDb<TextDb>::snapshot_extension);
  if (!fs::exists(snapshot_path)) {
    // try *.userdb.kct.snapshot
    std::string legacy_snapshot_file =
        dict_name + UserDb<TreeDb>::extension + ".snapshot";
    snapshot_path = dir / legacy_snapshot_file;
    if (!fs::exists(snapshot_path)) {
      return;  // not found
    }
  }
  LOG(INFO) << "snapshot exists, trying to restore db '" << dict_name << "'.";
  if (db_->Restore(snapshot_path.string())) {
    LOG(INFO) << "restored db '" << dict_name << "' from snapshot.";
  }
}

UserDbRecoveryTask* UserDbRecoveryTaskComponent::Create(TaskInitializer arg) {
  try {
    auto db = boost::any_cast<shared_ptr<Db>>(arg);
    return new UserDbRecoveryTask(db);
  }
  catch (const boost::bad_any_cast&) {
    return NULL;
  }
}

}  // namespace rime
