//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/service.h>
#include <rime/dict/user_db.h>

namespace rime {

// UserDb members

bool UserDb::CreateMetadata() {
  Deployer& deployer(Service::instance().deployer());
  return TreeDb::CreateMetadata() &&
      MetaUpdate("/user_id", deployer.user_id);
}

bool UserDb::IsUserDb() {
  std::string db_type;
  return MetaFetch("/db_type", &db_type) && (db_type == "userdb");
}

const std::string UserDb::GetDbName() {
  std::string name;
  if (!MetaFetch("/db_name", &name))
    return name;
  boost::erase_last(name, ".kct");
  boost::erase_last(name, ".userdb");
  return name;
}

const std::string UserDb::GetUserId() {
  std::string user_id("unknown");
  MetaFetch("/user_id", &user_id);
  return user_id;
}

const std::string UserDb::GetRimeVersion() {
  std::string version;
  MetaFetch("/rime_version", &version);
  return version;
}

TickCount UserDb::GetTickCount() {
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

}  // namespace rime
