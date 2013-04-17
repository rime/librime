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

const std::string UserDb<TextDb>::extension(".userdb.txt");
const std::string UserDb<TreeDb>::extension(".userdb.kct");

UserDb<TextDb>::UserDb(const std::string& name)
    : TextDb(name + extension, "userdb", 2) {
}

UserDb<TreeDb>::UserDb(const std::string& name)
    : TreeDb(name + extension, "userdb") {
}

template <class BaseDb>
bool UserDb<BaseDb>::CreateMetadata() {
  Deployer& deployer(Service::instance().deployer());
  return BaseDb::CreateMetadata() &&
      MetaUpdate("/user_id", deployer.user_id);
}

template <class BaseDb>
bool UserDb<BaseDb>::IsUserDb() {
  std::string db_type;
  return MetaFetch("/db_type", &db_type) && (db_type == "userdb");
}

template <class BaseDb>
const std::string UserDb<BaseDb>::GetDbName() {
  std::string name;
  if (!MetaFetch("/db_name", &name))
    return name;
  boost::erase_last(name, extension);
  return name;
}

template <class BaseDb>
const std::string UserDb<BaseDb>::GetUserId() {
  std::string user_id("unknown");
  MetaFetch("/user_id", &user_id);
  return user_id;
}

template <class BaseDb>
const std::string UserDb<BaseDb>::GetRimeVersion() {
  std::string version;
  MetaFetch("/rime_version", &version);
  return version;
}

template <class BaseDb>
TickCount UserDb<BaseDb>::GetTickCount() {
  std::string tick;
  if (MetaFetch("/tick", &tick)) {
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
