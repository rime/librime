//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>
#include <rime/lever/deployment_tasks.h>

using namespace rime;

static void rime_levers_initialize() {
  LOG(INFO) << "registering components from module 'levers'.";
  Registry& r = Registry::instance();

  // deployment tools
  r.Register("detect_modifications", new Component<DetectModifications>);
  r.Register("installation_update", new Component<InstallationUpdate>);
  r.Register("workspace_update", new Component<WorkspaceUpdate>);
  r.Register("schema_update", new Component<SchemaUpdate>);
  r.Register("config_file_update", new Component<ConfigFileUpdate>);
  r.Register("prebuild_all_schemas", new Component<PrebuildAllSchemas>);
  r.Register("user_dict_upgrade", new Component<UserDictUpgrade>);
  r.Register("cleanup_trash", new Component<CleanupTrash>);
  r.Register("user_dict_sync", new Component<UserDictSync>);
  r.Register("backup_config_files", new Component<BackupConfigFiles>);
  r.Register("clean_old_log_files", new Component<CleanOldLogFiles>);
}

static void rime_levers_finalize() {}

#include <rime_levers_api.h>
// defines rime_levers_get_api()
#include "levers_api_impl.h"

RIME_REGISTER_CUSTOM_MODULE(levers) {
  module->get_api = rime_levers_get_api;
}
