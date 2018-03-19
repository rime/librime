//
// Copyleft RIME Developers
// License: GPLv3
//
// 2014-12-10 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>
#include <rime/dict/user_db.h>
#include "tree_db.h"

static void rime_legacy_initialize() {
  using namespace rime;

  LOG(INFO) << "registering components from module 'legacy'.";
  Registry& r = Registry::instance();

  r.Register("treedb", new UserDbComponent<legacy::TreeDb>);
  r.Register("legacy_userdb", new UserDbComponent<legacy::TreeDb>);
}

static void rime_legacy_finalize() {
}

RIME_REGISTER_MODULE(legacy)
