//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/service.h>
#include <rime/dict/text_db.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/user_db.h>

namespace rime {

UserDbValue::UserDbValue()
    : commits(0), dee(0.0), tick(0) {
}

UserDbValue::UserDbValue(const std::string& value)
    : commits(0), dee(0.0), tick(0) {
  Unpack(value);
}

const std::string UserDbValue::Pack() const {
  return boost::str(boost::format("c=%1% d=%2% t=%3%") %
                    commits % dee % tick);
}

bool UserDbValue::Unpack(const std::string &value) {
  std::vector<std::string> kv;
  boost::split(kv, value, boost::is_any_of(" "));
  BOOST_FOREACH(const std::string &k_eq_v, kv) {
    size_t eq = k_eq_v.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string k(k_eq_v.substr(0, eq));
    const std::string v(k_eq_v.substr(eq + 1));
    try {
      if (k == "c") {
        commits = boost::lexical_cast<int>(v);
      }
      else if (k == "d") {
        dee = (std::min)(200.0, boost::lexical_cast<double>(v));
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

template <>
const std::string UserDb<TextDb>::extension(".userdb.txt");

template <>
const std::string UserDb<TreeDb>::extension(".userdb.kct");

// key ::= code <space> <Tab> phrase

static bool userdb_entry_parser(const Tsv& row,
                                std::string* key,
                                std::string* value) {
  if (row.size() < 2 ||
      row[0].empty() || row[1].empty()) {
    return false;
  }
  std::string code(row[0]);
  if (code[code.length() - 1] != ' ')
    code += ' ';
  *key = code + "\t" + row[1];
  if (row.size() >= 3)
    *value = row[2];
  else
    value->clear();
  return true;
}

static bool userdb_entry_formatter(const std::string& key,
                                   const std::string& value,
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
UserDb<TextDb>::UserDb(const std::string& name)
    : TextDb(name + extension, "userdb", plain_userdb_format) {
}

template <>
UserDb<TreeDb>::UserDb(const std::string& name)
    : TreeDb(name + extension, "userdb") {
}

template <class BaseDb>
bool UserDb<BaseDb>::CreateMetadata() {
  Deployer& deployer(Service::instance().deployer());
  return BaseDb::CreateMetadata() &&
      BaseDb::MetaUpdate("/user_id", deployer.user_id);
}

template <class BaseDb>
bool UserDb<BaseDb>::IsUserDb() {
  std::string db_type;
  return BaseDb::MetaFetch("/db_type", &db_type) && (db_type == "userdb");
}

template <class BaseDb>
const std::string UserDb<BaseDb>::GetDbName() {
  std::string name;
  if (!BaseDb::MetaFetch("/db_name", &name))
    return name;
  boost::erase_last(name, extension);
  return name;
}

template <class BaseDb>
const std::string UserDb<BaseDb>::GetUserId() {
  std::string user_id("unknown");
  BaseDb::MetaFetch("/user_id", &user_id);
  return user_id;
}

template <class BaseDb>
const std::string UserDb<BaseDb>::GetRimeVersion() {
  std::string version;
  BaseDb::MetaFetch("/rime_version", &version);
  return version;
}

template <class BaseDb>
TickCount UserDb<BaseDb>::GetTickCount() {
  std::string tick;
  if (UserDb::MetaFetch("/tick", &tick)) {
    try {
      return boost::lexical_cast<TickCount>(tick);
    }
    catch (...) {
    }
  }
  return 1;
}

template class UserDb<TextDb>;
template class UserDb<TreeDb>;

}  // namespace rime
