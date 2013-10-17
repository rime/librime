//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <boost/filesystem.hpp>
#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>
#include <rime/service.h>

// built-in components
#include <rime/config.h>
#include <rime/dict/table_db.h>
#include <rime/dict/text_db.h>
#include <rime/dict/tree_db.h>
#include <rime/dict/user_db.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/dict/user_dictionary.h>

static void rime_core_initialize() {
  using namespace rime;

  LOG(INFO) << "registering core components";
  Registry &r = Registry::instance();

  boost::filesystem::path user_data_dir =
      Service::instance().deployer().user_data_dir;
  boost::filesystem::path config_path = user_data_dir / "%s.yaml";
  boost::filesystem::path schema_path = user_data_dir / "%s.schema.yaml";
  r.Register("config", new ConfigComponent(config_path.string()));
  r.Register("schema_config", new ConfigComponent(schema_path.string()));

  r.Register("tabledb", new Component<TableDb>);
  r.Register("stabledb", new Component<StableDb>);
  r.Register("plain_userdb", new Component<UserDb<TextDb> >);
  r.Register("userdb", new Component<UserDb<TreeDb> >);

  r.Register("dictionary", new DictionaryComponent);
  r.Register("reverse_lookup_dictionary",
             new ReverseLookupDictionaryComponent);
  r.Register("user_dictionary", new UserDictionaryComponent);
}

static void rime_core_finalize() {
  // registered components have been automatically destroyed prior to this call
}

RimeModule* rime_core_module_init() {
  static RimeModule s_module = {0};
  if (!s_module.data_size) {
    RIME_STRUCT_INIT(RimeModule, s_module);
    s_module.initialize = rime_core_initialize;
    s_module.finalize = rime_core_finalize;
  }
  return &s_module;
}
