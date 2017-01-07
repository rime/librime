//
// Copyright RIME Developers
// Distributed under the BSD License
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
#include <rime/dict/user_db.h>
#include <rime/lever/user_dict_manager.h>

namespace fs = boost::filesystem;

namespace rime {

UserDictManager::UserDictManager(Deployer* deployer)
    : deployer_(deployer),
      user_db_component_(UserDb::Require("userdb")) {
  if (deployer) {
    path_ = deployer->user_data_dir;
  }
}

void UserDictManager::GetUserDictList(UserDictList* user_dict_list,
                                      UserDb::Component* component) {
  if (!user_dict_list)
    return;
  if (!component) {
    component = user_db_component_;
  }
  user_dict_list->clear();
  if (!fs::exists(path_) || !fs::is_directory(path_)) {
    LOG(INFO) << "directory '" << path_.string() << "' does not exist.";
    return;
  }
  for (fs::directory_iterator it(path_), end; it != end; ++it) {
    string name = it->path().filename().string();
    if (boost::ends_with(name, component->extension())) {
      boost::erase_last(name, component->extension());
      user_dict_list->push_back(name);
    }
  }
}

bool UserDictManager::Backup(const string& dict_name) {
  the<Db> db(user_db_component_->Create(dict_name));
  if (!db->OpenReadOnly())
    return false;
  if (UserDbHelper(db).GetUserId() != deployer_->user_id) {
    LOG(INFO) << "user id not match; recreating metadata in " << dict_name;
    if (!db->Close() || !db->Open() || !db->CreateMetadata()) {
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
  string snapshot_file =
      dict_name + UserDbFormat<TextDb>::snapshot_extension;
  return db->Backup((dir / snapshot_file).string());
}

bool UserDictManager::Restore(const string& snapshot_file) {
  the<Db> temp(user_db_component_->Create(".temp"));
  if (temp->Exists())
    temp->Remove();
  if (!temp->Open())
    return false;
  BOOST_SCOPE_EXIT( (&temp) )
  {
    temp->Close();
    temp->Remove();
  }
  BOOST_SCOPE_EXIT_END
  if (!temp->Restore(snapshot_file))
    return false;
  if (!UserDbHelper(temp).IsUserDb())
    return false;
  string db_name = UserDbHelper(temp).GetDbName();
  if (db_name.empty())
    return false;
  the<Db> dest(user_db_component_->Create(db_name));
  if (!dest->Open())
    return false;
  BOOST_SCOPE_EXIT( (&dest) )
  {
    dest->Close();
  } BOOST_SCOPE_EXIT_END
  LOG(INFO) << "merging '" << snapshot_file
            << "' from " << UserDbHelper(temp).GetUserId()
            << " into userdb '" << db_name << "'...";
  DbSource source(temp.get());
  UserDbMerger merger(dest.get());
  source >> merger;
  return true;
}

int UserDictManager::Export(const string& dict_name,
                            const string& text_file) {
  the<Db> db(user_db_component_->Create(dict_name));
  if (!db->OpenReadOnly())
    return -1;
  BOOST_SCOPE_EXIT( (&db) )
  {
    db->Close();
  }
  BOOST_SCOPE_EXIT_END
  if (!UserDbHelper(db).IsUserDb())
    return -1;
  TsvWriter writer(text_file, TableDb::format.formatter);
  writer.file_description = "Rime user dictionary export";
  DbSource source(db.get());
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

int UserDictManager::Import(const string& dict_name,
                            const string& text_file) {
  the<Db> db(user_db_component_->Create(dict_name));
  if (!db->Open())
    return -1;
  BOOST_SCOPE_EXIT( (&db) )
  {
    db->Close();
  }
  BOOST_SCOPE_EXIT_END
  if (!UserDbHelper(db).IsUserDb())
    return -1;
  TsvReader reader(text_file, TableDb::format.parser);
  UserDbImporter importer(db.get());
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

bool UserDictManager::UpgradeUserDict(const string& dict_name) {
  UserDb::Component* legacy_component = UserDb::Require("legacy_userdb");
  if (!legacy_component)
    return true;
  the<Db> legacy_db(legacy_component->Create(dict_name));
  if (!legacy_db->Exists())
    return true;
  if (!legacy_db->OpenReadOnly() || !UserDbHelper(legacy_db).IsUserDb())
    return false;
  LOG(INFO) << "upgrading user dict '" << dict_name << "'.";
  fs::path trash = fs::path(deployer_->user_data_dir) / "trash";
  if (!fs::exists(trash)) {
    boost::system::error_code ec;
    if (!fs::create_directories(trash, ec)) {
      LOG(ERROR) << "error creating directory '" << trash.string() << "'.";
      return false;
    }
  }
  string snapshot_file =
      dict_name + UserDbFormat<TextDb>::snapshot_extension;
  fs::path snapshot_path = trash / snapshot_file;
  return legacy_db->Backup(snapshot_path.string()) &&
         legacy_db->Close() &&
         legacy_db->Remove() &&
         Restore(snapshot_path.string());
}

bool UserDictManager::Synchronize(const string& dict_name) {
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
  // *.userdb.txt
  string snapshot_file =
      dict_name + UserDbFormat<TextDb>::snapshot_extension;
  for (fs::directory_iterator it(sync_dir), end; it != end; ++it) {
    if (!fs::is_directory(it->path()))
      continue;
    fs::path file_path = it->path() / snapshot_file;
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
  for (const string& dict_name : user_dicts) {
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
