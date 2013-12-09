//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-03-23 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <rime/common.h>
#include <rime/deployer.h>
#include <rime/algo/utilities.h>
#include <rime/dict/db_utils.h>
#include <rime/dict/table_db.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/user_db.h>
#include <rime/lever/user_dict_manager.h>

namespace fs = boost::filesystem;

namespace rime {

UserDictManager::UserDictManager(Deployer* deployer)
    : deployer_(deployer) {
  if (deployer) {
    path_ = deployer->user_data_dir;
  }
}

void UserDictManager::GetUserDictList(UserDictList* user_dict_list) {
  if (!user_dict_list) return;
  user_dict_list->clear();
  if (!fs::exists(path_) || !fs::is_directory(path_)) {
    LOG(INFO) << "directory '" << path_.string() << "' does not exist.";
    return;
  }
  fs::directory_iterator it(path_);
  fs::directory_iterator end;
  for (; it != end; ++it) {
    std::string name(it->path().filename().string());
    if (boost::ends_with(name, UserDb<TreeDb>::extension)) {
      boost::erase_last(name, UserDb<TreeDb>::extension);
      user_dict_list->push_back(name);
    }
  }
}

bool UserDictManager::Backup(const std::string& dict_name) {
  UserDb<TreeDb> db(dict_name);
  if (!db.OpenReadOnly())
    return false;
  if (db.GetUserId() != deployer_->user_id) {
    LOG(INFO) << "user id not match; recreating metadata in " << dict_name;
    if (!db.Close() || !db.Open() || !db.CreateMetadata()) {
      LOG(ERROR) << "failed to recreate metadata in " << dict_name;
      return false;
    }
  }
  boost::filesystem::path dir(deployer_->user_data_sync_dir());
  if (!boost::filesystem::exists(dir)) {
    if (!boost::filesystem::create_directories(dir)) {
      LOG(ERROR) << "error creating directory '" << dir.string() << "'.";
      return false;
    }
  }
  std::string snapshot_file =
      dict_name + UserDb<TextDb>::snapshot_extension;
  return db.Backup((dir / snapshot_file).string());
}

bool UserDictManager::Restore(const std::string& snapshot_file) {
  UserDb<TreeDb> temp(".temp");
  if (temp.Exists())
    temp.Remove();
  if (!temp.Open())
    return false;
  BOOST_SCOPE_EXIT( (&temp) )
  {
    temp.Close();
    temp.Remove();
  } BOOST_SCOPE_EXIT_END
  if (!temp.Restore(snapshot_file))
    return false;
  if (!temp.IsUserDb())
    return false;
  std::string db_name(temp.GetDbName());
  if (db_name.empty())
    return false;
  UserDb<TreeDb> dest(db_name);
  if (!dest.Open())
    return false;
  BOOST_SCOPE_EXIT( (&dest) )
  {
    dest.Close();
  } BOOST_SCOPE_EXIT_END
  LOG(INFO) << "merging '" << snapshot_file
            << "' from " << temp.GetUserId()
            << " into userdb '" << db_name << "'...";
  DbSource source(&temp);
  UserDbMerger merger(&dest);
  source >> merger;
  return true;
}

int UserDictManager::Export(const std::string& dict_name,
                            const std::string& text_file) {
  UserDb<TreeDb> db(dict_name);
  if (!db.OpenReadOnly())
    return -1;
  BOOST_SCOPE_EXIT( (&db) )
  {
    db.Close();
  } BOOST_SCOPE_EXIT_END
  if (!db.IsUserDb())
    return -1;
  TsvWriter writer(text_file, TableDb::format.formatter);
  writer.file_description = "Rime user dictionary export";
  DbSource source(&db);
  int num_entries = 0;
  try {
    num_entries = writer << source;
  }
  catch (std::exception& ex) {
    LOG(ERROR) << ex.what();
    return -1;
  }
  DLOG(INFO) << num_entries << " entries exported.";
  return num_entries;
}

int UserDictManager::Import(const std::string& dict_name,
                            const std::string& text_file) {
  UserDb<TreeDb> db(dict_name);
  if (!db.Open())
    return -1;
  BOOST_SCOPE_EXIT( (&db) )
  {
    db.Close();
  } BOOST_SCOPE_EXIT_END
  if (!db.IsUserDb())
    return -1;
  TsvReader reader(text_file, TableDb::format.parser);
  UserDbImporter importer(&db);
  int num_entries = 0;
  try {
    num_entries = reader >> importer;
  }
  catch (std::exception& ex) {
    LOG(ERROR) << ex.what();
    return -1;
  }
  DLOG(INFO) << num_entries << " entries imported.";
  return num_entries;
}

bool UserDictManager::UpgradeUserDict(const std::string& dict_name) {
  UserDb<TreeDb> db(dict_name);
  if (!db.OpenReadOnly())
    return false;
  if (!db.IsUserDb())
    return false;
  if (CompareVersionString(db.GetRimeVersion(), "0.9.7") < 0) {
    // fix invalid keys created by a buggy version of Import()
    LOG(INFO) << "upgrading user dict '" << dict_name << "'.";
    fs::path trash = fs::path(deployer_->user_data_dir) / "trash";
    if (!fs::exists(trash)) {
      boost::system::error_code ec;
      if (!fs::create_directories(trash, ec)) {
        LOG(ERROR) << "error creating directory '" << trash.string() << "'.";
        return false;
      }
    }
    std::string snapshot_file =
        dict_name + UserDb<TextDb>::snapshot_extension;
    fs::path snapshot_path = trash / snapshot_file;
    return db.Backup(snapshot_path.string()) &&
        db.Close() &&
        db.Remove() &&
        Restore(snapshot_path.string());
  }
  return true;
}

bool UserDictManager::Synchronize(const std::string& dict_name) {
  LOG(INFO) << "synchronize user dict '" << dict_name << "'.";
  bool success = true;
  fs::path sync_dir(deployer_->sync_dir);
  if (!fs::exists(sync_dir)) {
    boost::system::error_code ec;
    if (!fs::create_directories(sync_dir, ec)) {
      LOG(ERROR) << "error creating directory '" << sync_dir.string() << "'.";
      return false;
    }
  }
  fs::directory_iterator it(sync_dir);
  fs::directory_iterator end;
  // *.userdb.txt
  std::string snapshot_file =
      dict_name + UserDb<TextDb>::snapshot_extension;
  // *.userdb.kct.snapshot
  std::string legacy_snapshot_file =
      dict_name + UserDb<TreeDb>::extension + ".snapshot";
  for (; it != end; ++it) {
    if (!fs::is_directory(it->path()))
      continue;
    fs::path file_path = it->path() / snapshot_file;
    fs::path legacy_path = it->path() / legacy_snapshot_file;
    if (!fs::exists(file_path) && fs::exists(legacy_path)) {
      file_path = legacy_path;
    }
    if (fs::exists(file_path)) {
      LOG(INFO) << "merging snapshot file: " << file_path.string();
      if (!Restore(file_path.string())) {
        LOG(ERROR) << "failed to merge snapshot file: " << file_path.string();
        success = false;
      }
    }
  }
  if (!Backup(dict_name)) {
    LOG(ERROR) << "error backing up user dict '" << dict_name << "'.";
    success = false;
  }
  return success;
}

bool UserDictManager::SynchronizeAll() {
  UserDictList user_dicts;
  GetUserDictList(&user_dicts);
  LOG(INFO) << "synchronizing " << user_dicts.size() << " user dicts.";
  int failure = 0;
  for (const std::string& dict_name : user_dicts) {
    if (!Synchronize(dict_name))
      ++failure;
  }
  if (failure) {
    LOG(ERROR) << "failed synchronizing "
               << failure << "/" << user_dicts.size() << " user dicts.";
  }
  return !failure;
}

}  // namespace rime
