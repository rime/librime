//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>

#include <rime/lever/deployment_tasks.h>
#include <rime/lever/userdb_recovery_task.h>

static void rime_levers_initialize() {
  using namespace rime;

  LOG(INFO) << "registering components from module 'levers'";
  Registry &r = Registry::instance();

  // deployment tools
  r.Register("installation_update", new Component<InstallationUpdate>);
  r.Register("workspace_update", new Component<WorkspaceUpdate>);
  r.Register("schema_update", new Component<SchemaUpdate>);
  r.Register("config_file_update", new Component<ConfigFileUpdate>);
  r.Register("prebuild_all_schemas", new Component<PrebuildAllSchemas>);
  r.Register("user_dict_upgration", new Component<UserDictUpgration>);
  r.Register("cleanup_trash", new Component<CleanupTrash>);
  r.Register("user_dict_sync", new Component<UserDictSync>);
  r.Register("backup_config_files", new Component<BackupConfigFiles>);
  r.Register("clean_old_log_files", new Component<CleanOldLogFiles>);

  r.Register("userdb_recovery_task", new UserDbRecoveryTaskComponent);
}

static void rime_levers_finalize() {
}

RimeModule* rime_levers_module_init() {
  static RimeModule s_module = {0};
  if (!s_module.data_size) {
    RIME_STRUCT_INIT(RimeModule, s_module);
    s_module.initialize = rime_levers_initialize;
    s_module.finalize = rime_levers_finalize;
  }
  return &s_module;
}
