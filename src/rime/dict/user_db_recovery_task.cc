//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <boost/scope_exit.hpp>
#include <rime/deployer.h>
#include <rime/dict/db.h>
#include <rime/dict/user_db.h>
#include <rime/dict/user_db_recovery_task.h>

namespace rime {

UserDbRecoveryTask::UserDbRecoveryTask(an<Db> db) : db_(db) {
  if (db_) {
    db_->disable();
  }
}

bool UserDbRecoveryTask::Run(Deployer* deployer) {
  if (!db_) {
    return false;
  }
  BOOST_SCOPE_EXIT((&db_)) {
    db_->enable();
  }
  BOOST_SCOPE_EXIT_END

  if (db_->loaded()) {
    LOG(WARNING) << "cannot recover loaded db '" << db_->name() << "'.";
    return false;
  }
  auto r = As<Recoverable>(db_);
  if (r && r->Recover()) {
    return true;
  }
  // repair didn't work on the damaged db file; remove and recreate it
  LOG(INFO) << "recreating db file.";
  if (db_->Exists()) {
    std::error_code ec;
    std::filesystem::rename(db_->file_path(),
                            path(db_->file_path()).concat(".old"), ec);
    if (ec && !db_->Remove()) {
      LOG(ERROR) << "Error removing db file '" << db_->file_path() << "'.";
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
  UserDb::Component* component = UserDb::Require("userdb");
  if (!component || !UserDbHelper(db_).IsUserDb())
    return;
  string dict_name(db_->name());
  boost::erase_last(dict_name, component->extension());
  // locate snapshot file
  const path& dir(deployer->user_data_sync_dir());
  // try *.userdb.txt
  path snapshot_path = dir / (dict_name + UserDb::snapshot_extension());
  if (!std::filesystem::exists(snapshot_path)) {
    // try *.userdb.*.snapshot
    string legacy_snapshot_file =
        dict_name + component->extension() + ".snapshot";
    snapshot_path = dir / legacy_snapshot_file;
    if (!std::filesystem::exists(snapshot_path)) {
      return;  // not found
    }
  }
  LOG(INFO) << "snapshot exists, trying to restore db '" << dict_name << "'.";
  if (db_->Restore(snapshot_path)) {
    LOG(INFO) << "restored db '" << dict_name << "' from snapshot.";
  }
}

UserDbRecoveryTask* UserDbRecoveryTaskComponent::Create(TaskInitializer arg) {
  try {
    auto db = std::any_cast<an<Db>>(arg);
    return new UserDbRecoveryTask(db);
  } catch (const std::bad_any_cast&) {
    return NULL;
  }
}

}  // namespace rime
