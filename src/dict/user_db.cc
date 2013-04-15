//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-11-02 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <rime_version.h>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/dict/user_db.h>
#include <rime/algo/syllabifier.h>

namespace rime {

// UserDb members

bool UserDb::IsUserDb() {
  std::string db_type;
  return Fetch("\x01/db_type", &db_type) && db_type == "userdb";
}

const std::string UserDb::GetDbName() {
  std::string name;
  if (!Fetch("\x01/db_name", &name))
    return name;
  boost::erase_last(name, ".kct");
  boost::erase_last(name, ".userdb");
  return name;
}

const std::string UserDb::GetUserId() {
  std::string user_id("unknown");
  Fetch("\x01/user_id", &user_id);
  return user_id;
}

TickCount UserDb::GetTickCount() {
  std::string tick;
  if (Fetch("\x01/tick", &tick)) {
    try {
      return boost::lexical_cast<TickCount>(tick);
    }
    catch (...) {
    }
  }
  return 1;
}

}  // namespace rime
