//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/service.h>
#include <rime/algo/dynamics.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_db.h>

namespace rime {

UserDbValue::UserDbValue(const string& value) {
  Unpack(value);
}

string UserDbValue::Pack() const {
  return boost::str(boost::format("c=%1% d=%2% t=%3%") %
                    commits % dee % tick);
}

bool UserDbValue::Unpack(const string& value) {
  vector<string> kv;
  boost::split(kv, value, boost::is_any_of(" "));
  for (const string& k_eq_v : kv) {
    size_t eq = k_eq_v.find('=');
    if (eq == string::npos)
      continue;
    string k(k_eq_v.substr(0, eq));
    string v(k_eq_v.substr(eq + 1));
    try {
      if (k == "c") {
        commits = boost::lexical_cast<int>(v);
      }
      else if (k == "d") {
        dee = (std::min)(10000.0, boost::lexical_cast<double>(v));
      }
      else if (k == "t") {
        tick = boost::lexical_cast<TickCount>(v);
      }
    }
    catch (...) {
      LOG(ERROR) << "failed in parsing key-value from userdb entry '"
                 << k_eq_v << "'.";
      return false;
    }
  }
  return true;
}

static const string plain_userdb_extension(".userdb.txt");

template <>
string UserDbComponent<TextDb>::extension() const {
  return plain_userdb_extension;
}

template <>
string UserDbComponent<TextDb>::snapshot_extension() const {
  return plain_userdb_extension;
}

// key ::= code <space> <Tab> phrase

static bool userdb_entry_parser(const Tsv& row,
                                string* key,
                                string* value) {
  if (row.size() < 2 ||
      row[0].empty() || row[1].empty()) {
    return false;
  }
  string code(row[0]);
  // fix invalid keys created by a buggy version
  if (code[code.length() - 1] != ' ')
    code += ' ';
  *key = code + "\t" + row[1];
  if (row.size() >= 3)
    *value = row[2];
  else
    value->clear();
  return true;
}

static bool userdb_entry_formatter(const string& key,
                                   const string& value,
                                   Tsv* tsv) {
  Tsv& row(*tsv);
  boost::algorithm::split(row, key,
                          boost::algorithm::is_any_of("\t"));
  if (row.size() != 2 ||
      row[0].empty() || row[1].empty())
    return false;
  row.push_back(value);
  return true;
}

static TextFormat plain_userdb_format = {
  userdb_entry_parser,
  userdb_entry_formatter,
  "Rime user dictionary",
};

template <>
UserDbWrapper<TextDb>::UserDbWrapper(const string& db_name)
    : TextDb(db_name, "userdb", plain_userdb_format) {
}

bool UserDbHelper::UpdateUserInfo() {
  Deployer& deployer(Service::instance().deployer());
  return db_->MetaUpdate("/user_id", deployer.user_id);
}

bool UserDbHelper::IsUniformFormat(const string& file_name) {
  return boost::ends_with(file_name, plain_userdb_extension);
}

bool UserDbHelper::UniformBackup(const string& snapshot_file) {
  LOG(INFO) << "backing up userdb '" << db_->name() << "' to "
            << snapshot_file;
  TsvWriter writer(snapshot_file, plain_userdb_format.formatter);
  writer.file_description = plain_userdb_format.file_description;
  DbSource source(db_);
  try {
    writer << source;
  }
  catch (std::exception& ex) {
    LOG(ERROR) << ex.what();
    return false;
  }
  return true;
}

bool UserDbHelper::UniformRestore(const string& snapshot_file) {
  LOG(INFO) << "restoring userdb '" << db_->name() << "' from "
            << snapshot_file;
  TsvReader reader(snapshot_file, plain_userdb_format.parser);
  DbSink sink(db_);
  try {
    reader >> sink;
  }
  catch (std::exception& ex) {
    LOG(ERROR) << ex.what();
    return false;
  }
  return true;
}

bool UserDbHelper::IsUserDb() {
  string db_type;
  return db_->MetaFetch("/db_type", &db_type) && (db_type == "userdb");
}

string UserDbHelper::GetDbName() {
  string name;
  if (!db_->MetaFetch("/db_name", &name))
    return name;
  auto ext = boost::find_last(name, ".userdb");
  if (!ext.empty()) {
    // remove ".userdb.*"
    name.erase(ext.begin(), name.end());
  }
  return name;
}

string UserDbHelper::GetUserId() {
  string user_id("unknown");
  db_->MetaFetch("/user_id", &user_id);
  return user_id;
}

string UserDbHelper::GetRimeVersion() {
  string version;
  db_->MetaFetch("/rime_version", &version);
  return version;
}

static TickCount get_tick_count(Db* db) {
  string tick;
  if (db && db->MetaFetch("/tick", &tick)) {
    try {
      return boost::lexical_cast<TickCount>(tick);
    }
    catch (...) {
    }
  }
  return 1;
}

UserDbMerger::UserDbMerger(Db* db) : db_(db) {
  our_tick_ = get_tick_count(db);
  their_tick_ = 0;
  max_tick_ = our_tick_;
}

UserDbMerger::~UserDbMerger() {
  CloseMerge();
}

bool UserDbMerger::MetaPut(const string& key, const string& value) {
  if (key == "/tick") {
    try {
      their_tick_ = boost::lexical_cast<TickCount>(value);
      max_tick_ = (std::max)(our_tick_, their_tick_);
    }
    catch (...) {
    }
  }
  return true;
}

bool UserDbMerger::Put(const string& key, const string& value) {
  if (!db_) return false;
  UserDbValue v(value);
  if (v.tick < their_tick_) {
    v.dee = algo::formula_d(0, (double)their_tick_, v.dee, (double)v.tick);
  }
  UserDbValue o;
  string our_value;
  if (db_->Fetch(key, &our_value)) {
    o.Unpack(our_value);
  }
  if (o.tick < our_tick_) {
    o.dee = algo::formula_d(0, (double)our_tick_, o.dee, (double)o.tick);
  }
  if (std::abs(o.commits) < std::abs(v.commits))
      o.commits = v.commits;
  o.dee = (std::max)(o.dee, v.dee);
  o.tick = max_tick_;
  return db_->Update(key, o.Pack()) && ++merged_entries_;
}

void UserDbMerger::CloseMerge() {
  if (!db_ || !merged_entries_)
    return;
  Deployer& deployer(Service::instance().deployer());
  try {
    db_->MetaUpdate("/tick", boost::lexical_cast<string>(max_tick_));
    db_->MetaUpdate("/user_id", deployer.user_id);
  }
  catch (...) {
    LOG(ERROR) << "failed to update tick count.";
    return;
  }
  LOG(INFO) << "total " << merged_entries_ << " entries merged, tick = "
            << max_tick_;
  merged_entries_ = 0;
}

UserDbImporter::UserDbImporter(Db* db)
    : db_(db) {
}

bool UserDbImporter::MetaPut(const string& key, const string& value) {
  return true;
}

bool UserDbImporter::Put(const string& key, const string& value) {
  if (!db_) return false;
  UserDbValue v(value);
  UserDbValue o;
  string old_value;
  if (db_->Fetch(key, &old_value)) {
    o.Unpack(old_value);
  }
  if (v.commits > 0) {
    o.commits = (std::max)(o.commits, v.commits);
    o.dee = (std::max)(o.dee, v.dee);
  }
  else if (v.commits < 0) {  // mark as deleted
    o.commits = (std::min)(v.commits, -std::abs(o.commits));
  }
  return db_->Update(key, o.Pack());
}

}  // namespace rime
