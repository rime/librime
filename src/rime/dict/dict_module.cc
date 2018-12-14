//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>

#include <rime/dict/level_db.h>
#include <rime/dict/table_db.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_db.h>
#include <rime/dict/corrector.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/dict/user_dictionary.h>
#include <rime/dict/user_db_recovery_task.h>

static void rime_dict_initialize() {
  using namespace rime;

  LOG(INFO) << "registering components from module 'dict'.";
  Registry& r = Registry::instance();

  r.Register("tabledb", new Component<TableDb>);
  r.Register("stabledb", new Component<StableDb>);
  r.Register("plain_userdb", new UserDbComponent<TextDb>);
  r.Register("userdb", new UserDbComponent<LevelDb>);
  // NOTE: register a legacy_userdb component in your plugin if you wish to
  // upgrade userdbs from an old file format (eg. TreeDb) during maintenance.
  //r.Register("legacy_userdb", ...);

  r.Register("corrector", new CorrectorComponent);

  r.Register("dictionary", new DictionaryComponent);
  r.Register("reverse_lookup_dictionary",
             new ReverseLookupDictionaryComponent);
  r.Register("user_dictionary", new UserDictionaryComponent);

  r.Register("userdb_recovery_task", new UserDbRecoveryTaskComponent);
}

static void rime_dict_finalize() {
}

RIME_REGISTER_MODULE(dict)
