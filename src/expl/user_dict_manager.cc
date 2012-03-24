// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-03-23 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <rime/common.h>
#include <rime/deployer.h>
#include <rime/algo/dynamics.h>
#include <rime/dict/user_db.h>
#include <rime/dict/user_dictionary.h>
#include <rime/expl/user_dict_manager.h>

namespace fs = boost::filesystem;

namespace rime {

static bool IsUserDb(UserDb& db) {
  std::string db_type;
  return db.Fetch("\x01/db_type", &db_type) && db_type == "userdb";
}

static const std::string GetDbName(UserDb& db) {
  std::string name;
  if (!db.Fetch("\x01/db_name", &name))
    return name;
  boost::erase_last(name, ".kct");
  boost::erase_last(name, ".userdb");
  return name;
}

static TickCount GetTickCount(UserDb& db) {
  std::string tick;
  if (db.Fetch("\x01/tick", &tick)) {
    try {
      return boost::lexical_cast<TickCount>(tick);
    }
    catch (...) {
    }
  }
  return 1;
}

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
    EZLOGGERPRINT("Info: directory '%s' does not exist.",
                  path_.string().c_str());
    return;
  }
  fs::directory_iterator it(path_);
  fs::directory_iterator end;
  for (; it != end; ++it) {
    std::string name(it->path().filename().string());
    if (boost::ends_with(name, ".userdb.kct")) {
      boost::erase_last(name, ".userdb.kct");
      user_dict_list->push_back(name);
    }
  }
}

bool UserDictManager::Backup(const std::string& dict_name) {
  UserDb db(dict_name);
  if (!db.OpenReadOnly())
    return false;
  return db.Backup();
}

bool UserDictManager::Restore(const std::string& snapshot_file) {
  UserDb temp(".temp");
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
  if (!IsUserDb(temp))
    return false;
  std::string db_name(GetDbName(temp));
  if (db_name.empty())
    return false;
  UserDb dest(db_name);
  if (!dest.Open())
    return false;
  BOOST_SCOPE_EXIT( (&dest) )
  {
    dest.Close();
  } BOOST_SCOPE_EXIT_END
  EZLOGGERPRINT("merging '%s' into userdb '%s'...",
                snapshot_file.c_str(),
                db_name.c_str());
  TickCount tick_left = GetTickCount(dest);
  TickCount tick_right = GetTickCount(temp);
  TreeDbAccessor a(temp.Query(""));
  std::string key, left, right;
  int num_entries = 0;
  while (a.GetNextRecord(&key, &right)) {
    if (boost::starts_with(key, "\x01/"))  // skip metadata
      continue;
    int c = 0;
    double d = 0.0;
    TickCount t = 0;
    UserDictionary::UnpackValues(right, &c, &d, &t);
    if (t < tick_right)
      d = algo::formula_d(0, (double)t, d, (double)tick_right);
    if (dest.Fetch(key, &left)) {
      int c0 = 0;
      double d0 = 0.0;
      TickCount t0 = 0;
      UserDictionary::UnpackValues(left, &c0, &d0, &t0);
      if (t0 < tick_left)
        d0 = algo::formula_d(0, (double)t0, d0, (double)tick_left);
      c = (std::max)(c, c0);
      d = (std::max)(d, d0);
    }
    right = boost::str(boost::format("c=%1% d=%2% t=%3%") % 
                       c % d % tick_left);
    if (dest.Update(key, right))
      ++num_entries;
  }
  if (num_entries > 0) {
    try {
      dest.Update("\x01/tick", boost::lexical_cast<std::string>(tick_left));
    }
    catch (...) {
      EZLOGGERPRINT("Warning: failed to update tick count.");
    }
  }
  EZLOGGERPRINT("total %d entries imported, tick = %d.",
                num_entries, tick_left);
  return true;
}

int UserDictManager::Export(const std::string& dict_name,
                            const std::string& text_file) {
  UserDb db(dict_name);
  if (!db.OpenReadOnly())
    return -1;
  BOOST_SCOPE_EXIT( (&db) )
  {
    db.Close();
  } BOOST_SCOPE_EXIT_END
  if (!IsUserDb(db))
    return -1;
  std::ofstream fout(text_file);
  std::string key, value;
  int num_entries = 0;
  UserDbAccessor a(db.Query(""));
  while (a.GetNextRecord(&key, &value)) {
    if (boost::starts_with(key, "\x01/"))  // skip metadata
      continue;
    int c = 0;
    double d = 0.0;
    TickCount t = 0;
    UserDictionary::UnpackValues(value, &c, &d, &t);
    if (c < 0)  // deleted entry
      continue;
    fout << key << "\t" << c << std::endl;
    ++num_entries;
  }
  fout.close();
  return num_entries;
}

int UserDictManager::Import(const std::string& dict_name,
                            const std::string& text_file) {
  UserDb db(dict_name);
  if (!db.Open())
    return -1;
  BOOST_SCOPE_EXIT( (&db) )
  {
    db.Close();
  } BOOST_SCOPE_EXIT_END
  if (!IsUserDb(db))
    return -1;
  std::ifstream fin(text_file);
  std::string line, key, value;
  int num_entries = 0;
  while (getline(fin, line)) {
    // skip empty lines and comments
    if (line.empty() || line[0] == '#') continue;
    // read a dict entry
    std::vector<std::string> row;
    boost::algorithm::split(row, line,
                            boost::algorithm::is_any_of("\t"));
    if (row.size() < 2 || row[0].empty() || row[1].empty()) {
      EZLOGGERPRINT("Warning: invalid entry at #%d.", num_entries);
      continue;
    }
    key = row[0] + "\t" + row[1];
    int commits = 0;
    if (row.size() >= 3 && !row[2].empty()) {
      try {
        commits = boost::lexical_cast<int>(row[2]);
      }
      catch (...) {
      }
    }
    int c = 0;
    double d = 0.0;
    TickCount t = 0;
    if (db.Fetch(key, &value))
      UserDictionary::UnpackValues(value, &c, &d, &t);
    if (commits > 0)
      c = (std::max)(commits, c);
    else if (commits < 0)  // mark as deleted
      c = commits;
    value = boost::str(boost::format("c=%1% d=%2% t=%3%") %
                       c % d % t);
    if (db.Update(key, value))
      ++num_entries;
  }
  fin.close();
  return num_entries;
}

}  // namespace rime
