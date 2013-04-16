//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/service.h>
#include <rime/dict/text_db.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/user_db.h>

namespace rime {

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
