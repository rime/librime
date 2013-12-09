//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime_levers_api.h>
#include <rime/common.h>
#include <rime/registry.h>
#include <rime/service.h>

#include <rime/lever/custom_settings.h>
#include <rime/lever/deployment_tasks.h>
#include <rime/lever/switcher_settings.h>
#include <rime/lever/userdb_recovery_task.h>
#include <rime/lever/user_dict_manager.h>

static void rime_levers_initialize() {
  using namespace rime;

  LOG(INFO) << "registering components from module 'levers'.";
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

// implementation of levers api

static RimeCustomSettings* rime_levers_custom_settings_init(const char* config_id,
                                                            const char* generator_id) {
  return reinterpret_cast<RimeCustomSettings*>(
      new rime::CustomSettings(&rime::Service::instance().deployer(),
                               config_id, generator_id));
}

static void rime_levers_custom_settings_destroy(RimeCustomSettings* settings) {
  delete reinterpret_cast<rime::CustomSettings*>(settings);
}

static Bool rime_levers_load_settings(RimeCustomSettings* settings) {
  return Bool(reinterpret_cast<rime::CustomSettings*>(settings)->Load());
}

static Bool rime_levers_save_settings(RimeCustomSettings* settings) {
  return Bool(reinterpret_cast<rime::CustomSettings*>(settings)->Save());
}

static Bool rime_levers_customize_bool(RimeCustomSettings* settings,
                                       const char* key, bool value) {
  rime::ConfigItemPtr item = rime::New<rime::ConfigValue>(value);
  return reinterpret_cast<rime::CustomSettings*>(settings)->Customize(key, item);
}

static Bool rime_levers_customize_int(RimeCustomSettings* settings,
                                      const char* key, int value) {
  rime::ConfigItemPtr item = rime::New<rime::ConfigValue>(value);
  return reinterpret_cast<rime::CustomSettings*>(settings)->Customize(key, item);
}

static Bool rime_levers_customize_double(RimeCustomSettings* settings,
                                         const char* key, double value) {
  rime::ConfigItemPtr item = rime::New<rime::ConfigValue>(value);
  return reinterpret_cast<rime::CustomSettings*>(settings)->Customize(key, item);
}

static Bool rime_levers_customize_string(RimeCustomSettings* settings,
                                         const char* key, const char* value) {
  rime::ConfigItemPtr item = rime::New<rime::ConfigValue>(value);
  return reinterpret_cast<rime::CustomSettings*>(settings)->Customize(key, item);
}

static Bool rime_levers_is_first_run(RimeCustomSettings* settings) {
  return reinterpret_cast<rime::CustomSettings*>(settings)->IsFirstRun();
}

static Bool rime_levers_settings_is_modified(RimeCustomSettings* settings) {
  return reinterpret_cast<rime::CustomSettings*>(settings)->modified();
}

static Bool rime_levers_settings_get_config(RimeCustomSettings* settings, RimeConfig* config) {
  if (!config) return False;
  config->ptr = reinterpret_cast<rime::CustomSettings*>(settings)->config();
  return Bool(!!config->ptr);
}

static RimeSwitcherSettings* rime_levers_switcher_settings_init() {
  return reinterpret_cast<RimeSwitcherSettings*>(
      new rime::SwitcherSettings(&rime::Service::instance().deployer()));
}

static Bool rime_levers_get_available_schema_list(RimeSwitcherSettings* settings,
                                                  RimeSchemaList* list) {
  rime::SwitcherSettings* ss = reinterpret_cast<rime::SwitcherSettings*>(settings);
  list->size = 0;
  list->list = NULL;
  if (ss->available().empty()) {
    return False;
  }
  list->list = new RimeSchemaListItem[ss->available().size()];
  for (const rime::SchemaInfo& info : ss->available()) {
    RimeSchemaListItem& item(list->list[list->size]);
    item.schema_id = const_cast<char*>(info.schema_id.c_str());
    item.name = const_cast<char*>(info.name.c_str());
    item.reserved = const_cast<rime::SchemaInfo*>(&info);
    ++list->size;
  }
  return True;
}

static Bool rime_levers_get_selected_schema_list(RimeSwitcherSettings* settings,
                                                 RimeSchemaList* list) {
  rime::SwitcherSettings* ss = reinterpret_cast<rime::SwitcherSettings*>(settings);
  list->size = 0;
  list->list = NULL;
  if (ss->selection().empty()) {
    return False;
  }
  list->list = new RimeSchemaListItem[ss->selection().size()];
  for (const std::string& schema_id : ss->selection()) {
    RimeSchemaListItem& item(list->list[list->size]);
    item.schema_id = const_cast<char*>(schema_id.c_str());
    ++list->size;
  }
  return True;
}

static void rime_levers_schema_list_destroy(RimeSchemaList* list) {
  delete[] list->list;
  list->size = 0;
  list->list = NULL;
}

static const char* rime_levers_get_schema_id(RimeSchemaInfo* info) {
  rime::SchemaInfo* si = reinterpret_cast<rime::SchemaInfo*>(info);
  return si && !si->schema_id.empty() ? si->schema_id.c_str() : NULL;
}

static const char* rime_levers_get_schema_name(RimeSchemaInfo* info) {
  rime::SchemaInfo* si = reinterpret_cast<rime::SchemaInfo*>(info);
  return si && !si->name.empty() ? si->name.c_str() : NULL;
}

static const char* rime_levers_get_schema_version(RimeSchemaInfo* info) {
  rime::SchemaInfo* si = reinterpret_cast<rime::SchemaInfo*>(info);
  return si && !si->version.empty() ? si->version.c_str() : NULL;
}
static const char* rime_levers_get_schema_author(RimeSchemaInfo* info) {
  rime::SchemaInfo* si = reinterpret_cast<rime::SchemaInfo*>(info);
  return si && !si->author.empty() ? si->author.c_str() : NULL;
}

static const char* rime_levers_get_schema_description(RimeSchemaInfo* info) {
  rime::SchemaInfo* si = reinterpret_cast<rime::SchemaInfo*>(info);
  return si && !si->description.empty() ? si->description.c_str() : NULL;
}

static const char* rime_levers_get_schema_file_path(RimeSchemaInfo* info) {
  rime::SchemaInfo* si = reinterpret_cast<rime::SchemaInfo*>(info);
  return si && !si->file_path.empty() ? si->file_path.c_str() : NULL;
}

static Bool rime_levers_select_schemas(RimeSwitcherSettings* settings,
                                       const char* schema_id_list[],
                                       int count) {
  rime::SwitcherSettings* ss = reinterpret_cast<rime::SwitcherSettings*>(settings);
  rime::SwitcherSettings::Selection selection;
  for (int i = 0; i < count; ++i) {
    selection.push_back(schema_id_list[i]);
  }
  return ss->Select(selection);
}

static const char* rime_levers_get_hotkeys(RimeSwitcherSettings* settings) {
  rime::SwitcherSettings* ss = reinterpret_cast<rime::SwitcherSettings*>(settings);
  return !ss->hotkeys().empty() ? ss->hotkeys().c_str() : NULL;
}

static Bool rime_levers_set_hotkeys(RimeSwitcherSettings* settings, const char* hotkeys) {
  rime::SwitcherSettings* ss = reinterpret_cast<rime::SwitcherSettings*>(settings);
  return Bool(ss->SetHotkeys(hotkeys));
}

static Bool rime_levers_user_dict_iterator_init(RimeUserDictIterator* iter) {
  rime::UserDictManager mgr(&rime::Service::instance().deployer());
  rime::UserDictList* list = new rime::UserDictList;
  mgr.GetUserDictList(list);
  if (list->empty()) {
    delete list;
    return False;
  }
  iter->ptr = list;
  iter->i = 0;
  return True;
}

static void rime_levers_user_dict_iterator_destroy(RimeUserDictIterator* iter) {
  delete (rime::UserDictList*)iter->ptr;
  iter->ptr = NULL;
  iter->i = 0;
}

static const char* rime_levers_next_user_dict(RimeUserDictIterator* iter) {
  rime::UserDictList* list = reinterpret_cast<rime::UserDictList*>(iter->ptr);
  if (!list || iter->i >= list->size()) {
    return NULL;
  }
  return (*list)[iter->i++].c_str();
}

static Bool rime_levers_backup_user_dict(const char* dict_name) {
  rime::UserDictManager mgr(&rime::Service::instance().deployer());
  return Bool(mgr.Backup(dict_name));
}

static Bool rime_levers_restore_user_dict(const char* snapshot_file) {
  rime::UserDictManager mgr(&rime::Service::instance().deployer());
  return Bool(mgr.Restore(snapshot_file));
}

static int rime_levers_export_user_dict(const char* dict_name, const char* text_file) {
  rime::UserDictManager mgr(&rime::Service::instance().deployer());
  return mgr.Export(dict_name, text_file);
}

static int rime_levers_import_user_dict(const char* dict_name, const char* text_file) {
  rime::UserDictManager mgr(&rime::Service::instance().deployer());
  return mgr.Import(dict_name, text_file);
}

//

static RimeCustomApi* rime_levers_get_api() {
  static RimeLeversApi s_api = {0};
  if (!s_api.data_size) {
    RIME_STRUCT_INIT(RimeLeversApi, s_api);
    s_api.custom_settings_init = rime_levers_custom_settings_init;
    s_api.custom_settings_destroy = rime_levers_custom_settings_destroy;
    s_api.load_settings = rime_levers_load_settings;
    s_api.save_settings = rime_levers_save_settings;
    s_api.customize_bool = rime_levers_customize_bool;
    s_api.customize_int = rime_levers_customize_int;
    s_api.customize_double = rime_levers_customize_double;
    s_api.customize_string = rime_levers_customize_string;
    s_api.is_first_run = rime_levers_is_first_run;
    s_api.settings_is_modified = rime_levers_settings_is_modified;
    s_api.settings_get_config = rime_levers_settings_get_config;
    s_api.switcher_settings_init = rime_levers_switcher_settings_init;
    s_api.get_available_schema_list = rime_levers_get_available_schema_list;
    s_api.get_selected_schema_list = rime_levers_get_selected_schema_list;
    s_api.schema_list_destroy = rime_levers_schema_list_destroy;
    s_api.get_schema_id = rime_levers_get_schema_id;
    s_api.get_schema_name = rime_levers_get_schema_name;
    s_api.get_schema_version = rime_levers_get_schema_version;
    s_api.get_schema_author = rime_levers_get_schema_author;
    s_api.get_schema_description = rime_levers_get_schema_description;
    s_api.get_schema_file_path = rime_levers_get_schema_file_path;
    s_api.select_schemas = rime_levers_select_schemas;
    s_api.get_hotkeys = rime_levers_get_hotkeys;
    s_api.set_hotkeys = rime_levers_set_hotkeys;
    s_api.user_dict_iterator_init = rime_levers_user_dict_iterator_init;
    s_api.user_dict_iterator_destroy = rime_levers_user_dict_iterator_destroy;
    s_api.next_user_dict = rime_levers_next_user_dict;
    s_api.backup_user_dict = rime_levers_backup_user_dict;
    s_api.restore_user_dict = rime_levers_restore_user_dict;
    s_api.export_user_dict = rime_levers_export_user_dict;
    s_api.import_user_dict = rime_levers_import_user_dict;
  }
  return (RimeCustomApi*)&s_api;
}

RIME_REGISTER_CUSTOM_MODULE(levers) {
  module->get_api = rime_levers_get_api;
}
