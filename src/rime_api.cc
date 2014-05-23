//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-08-09 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/deployer.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/module.h>
#include <rime/registry.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/setup.h>
#include <rime/signature.h>
#include <rime_api.h>

// assuming member is a pointer in struct *p
#define PROVIDED(p, member) ((p) && RIME_STRUCT_HAS_MEMBER(*(p), (p)->member) && (p)->member)

static void setup_deployer(RimeTraits *traits) {
  if (!traits) return;
  rime::Deployer &deployer(rime::Service::instance().deployer());
  if (PROVIDED(traits, shared_data_dir))
    deployer.shared_data_dir = traits->shared_data_dir;
  if (PROVIDED(traits, user_data_dir))
    deployer.user_data_dir = traits->user_data_dir;
  if (PROVIDED(traits, distribution_name))
    deployer.distribution_name = traits->distribution_name;
  if (PROVIDED(traits, distribution_code_name))
    deployer.distribution_code_name = traits->distribution_code_name;
  if (PROVIDED(traits, distribution_version))
    deployer.distribution_version = traits->distribution_version;
}

RIME_API void RimeSetupLogging(const char* app_name) {
  rime::SetupLogging(app_name);
}

#if RIME_BUILD_SHARED_LIBS
#define rime_declare_module_dependencies()
#else
extern void rime_require_module_core();
extern void rime_require_module_dict();
extern void rime_require_module_gears();
extern void rime_require_module_levers();
// link to default modules explicitly when building static library.
static void rime_declare_module_dependencies() {
  rime_require_module_core();
  rime_require_module_dict();
  rime_require_module_gears();
  rime_require_module_levers();
}
#endif

RIME_API void RimeSetup(RimeTraits *traits) {
  rime_declare_module_dependencies();

  setup_deployer(traits);
  if (PROVIDED(traits, app_name)) {
    rime::SetupLogging(traits->app_name);
  }
}

RIME_API void RimeSetNotificationHandler(RimeNotificationHandler handler,
                                         void* context_object) {
  if (handler) {
    rime::Service::instance().SetNotificationHandler(
        boost::bind(handler, context_object, _1, _2, _3));
  }
  else {
    rime::Service::instance().ClearNotificationHandler();
  }
}

RIME_API void RimeInitialize(RimeTraits *traits) {
  setup_deployer(traits);
  rime::LoadModules(
      PROVIDED(traits, modules) ? traits->modules : rime::kDefaultModules);
  rime::Service::instance().StartService();
}

RIME_API void RimeFinalize() {
  RimeJoinMaintenanceThread();
  rime::Service::instance().StopService();
  rime::Registry::instance().Clear();
  rime::ModuleManager::instance().UnloadModules();
}

RIME_API Bool RimeStartMaintenance(Bool full_check) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  deployer.RunTask("clean_old_log_files");
  if (!deployer.RunTask("installation_update")) {
    return False;
  }
  if (!full_check) {
    rime::TaskInitializer args(
        std::make_pair<std::string, std::string>(
            "default.yaml", "config_version"));
    if (!deployer.RunTask("config_file_update", args)) {
      return False;
    }
    LOG(INFO) << "changes detected; starting maintenance.";
  }
  deployer.ScheduleTask("workspace_update");
  deployer.ScheduleTask("user_dict_upgration");
  deployer.ScheduleTask("cleanup_trash");
  deployer.StartMaintenance();
  return True;
}

RIME_API Bool RimeStartMaintenanceOnWorkspaceChange() {
  return RimeStartMaintenance(False);
}

RIME_API Bool RimeIsMaintenancing() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return Bool(deployer.IsMaintenanceMode());
}

RIME_API void RimeJoinMaintenanceThread() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  deployer.JoinMaintenanceThread();
}

// deployment

RIME_API void RimeDeployerInitialize(RimeTraits *traits) {
  setup_deployer(traits);
  rime::LoadModules(
      PROVIDED(traits, modules) ? traits->modules : rime::kDeployerModules);
}

RIME_API Bool RimePrebuildAllSchemas() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return Bool(deployer.RunTask("prebuild_all_schemas"));
}

RIME_API Bool RimeDeployWorkspace() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return Bool(deployer.RunTask("installation_update") &&
              deployer.RunTask("workspace_update") &&
              deployer.RunTask("user_dict_upgration") &&
              deployer.RunTask("cleanup_trash"));
}

RIME_API Bool RimeDeploySchema(const char *schema_file) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return Bool(deployer.RunTask("schema_update", std::string(schema_file)));
}

RIME_API Bool RimeDeployConfigFile(const char *file_name,
                                   const char *version_key) {
  rime::Deployer& deployer(rime::Service::instance().deployer());
  rime::TaskInitializer args(
      std::make_pair<std::string, std::string>(
          file_name, version_key));
  return Bool(deployer.RunTask("config_file_update", args));
}

RIME_API Bool RimeSyncUserData() {
  RimeCleanupAllSessions();
  rime::Deployer& deployer(rime::Service::instance().deployer());
  deployer.ScheduleTask("installation_update");
  deployer.ScheduleTask("backup_config_files");
  deployer.ScheduleTask("user_dict_sync");
  return Bool(deployer.StartMaintenance());
}

// session management

RIME_API RimeSessionId RimeCreateSession() {
  return rime::Service::instance().CreateSession();
}

RIME_API Bool RimeFindSession(RimeSessionId session_id) {
  return Bool(session_id && rime::Service::instance().GetSession(session_id));
}

RIME_API Bool RimeDestroySession(RimeSessionId session_id) {
  return Bool(rime::Service::instance().DestroySession(session_id));
}

RIME_API void RimeCleanupStaleSessions() {
  rime::Service::instance().CleanupStaleSessions();
}

RIME_API void RimeCleanupAllSessions() {
  rime::Service::instance().CleanupAllSessions();
}

// input

RIME_API Bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  return Bool(session->ProcessKey(rime::KeyEvent(keycode, mask)));
}

RIME_API Bool RimeCommitComposition(RimeSessionId session_id) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  return Bool(session->CommitComposition());
}

RIME_API void RimeClearComposition(RimeSessionId session_id) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return;
  session->ClearComposition();
}

// output

RIME_API Bool RimeGetContext(RimeSessionId session_id, RimeContext* context) {
  if (!context || context->data_size <= 0)
    return False;
  RIME_STRUCT_CLEAR(*context);
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Context *ctx = session->context();
  if (!ctx)
    return False;
  if (ctx->IsComposing()) {
    rime::Preedit preedit;
    ctx->GetPreedit(&preedit, ctx->get_option("soft_cursor"));
    context->composition.length = preedit.text.length();
    context->composition.preedit = new char[preedit.text.length() + 1];
    std::strcpy(context->composition.preedit, preedit.text.c_str());
    context->composition.cursor_pos = preedit.caret_pos;
    context->composition.sel_start = preedit.sel_start;
    context->composition.sel_end = preedit.sel_end;
    if (RIME_STRUCT_HAS_MEMBER(*context, context->commit_text_preview)) {
      std::string commit_text(ctx->GetCommitText());
      if (!commit_text.empty()) {
        context->commit_text_preview = new char[commit_text.length() + 1];
        std::strcpy(context->commit_text_preview, commit_text.c_str());
      }
    }
  }
  if (ctx->HasMenu()) {
    rime::Segment &seg(ctx->composition()->back());
    int page_size = 5;
    rime::Schema *schema = session->schema();
    if (schema)
      page_size = schema->page_size();
    int selected_index = seg.selected_index;
    int page_no = selected_index / page_size;
    boost::scoped_ptr<rime::Page> page(seg.menu->CreatePage(page_size, page_no));
    if (page) {
      context->menu.page_size = page_size;
      context->menu.page_no = page_no;
      context->menu.is_last_page = Bool(page->is_last_page);
      context->menu.highlighted_candidate_index = selected_index % page_size;
      int i = 0;
      context->menu.num_candidates = page->candidates.size();
      context->menu.candidates = new RimeCandidate[page->candidates.size()];
      BOOST_FOREACH(const boost::shared_ptr<rime::Candidate> &cand, page->candidates) {
        RimeCandidate* dest = &context->menu.candidates[i++];
        dest->text = new char[cand->text().length() + 1];
        std::strcpy(dest->text, cand->text().c_str());
        std::string comment(cand->comment());
        if (!comment.empty()) {
          dest->comment = new char[comment.length() + 1];
          std::strcpy(dest->comment, comment.c_str());
        }
        else {
          dest->comment = NULL;
        }
      }
      if (schema) {
        const std::string& select_keys(schema->select_keys());
        if (!select_keys.empty()) {
          context->menu.select_keys = new char[select_keys.length() + 1];
          std::strcpy(context->menu.select_keys, select_keys.c_str());
        }
      }
    }
  }
  return True;
}

RIME_API Bool RimeFreeContext(RimeContext* context) {
  if (!context || context->data_size <= 0)
    return False;
  delete[] context->composition.preedit;
  for (int i = 0; i < context->menu.num_candidates; ++i) {
    delete[] context->menu.candidates[i].text;
    delete[] context->menu.candidates[i].comment;
  }
  delete[] context->menu.candidates;
  delete[] context->menu.select_keys;
  if (RIME_STRUCT_HAS_MEMBER(*context, context->commit_text_preview)) {
    delete[] context->commit_text_preview;
  }
  RIME_STRUCT_CLEAR(*context);
  return True;
}

RIME_API Bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit) {
  if (!commit)
    return False;
  RIME_STRUCT_CLEAR(*commit);
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  const std::string& commit_text(session->commit_text());
  if (!commit_text.empty()) {
    commit->text = new char[commit_text.length() + 1];
    std::strcpy(commit->text, commit_text.c_str());
    session->ResetCommitText();
    return True;
  }
  return False;
}

RIME_API Bool RimeFreeCommit(RimeCommit* commit) {
  if (!commit)
    return False;
  delete[] commit->text;
  RIME_STRUCT_CLEAR(*commit);
  return True;
}

RIME_API Bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status) {
  if (!status || status->data_size <= 0)
    return False;
  RIME_STRUCT_CLEAR(*status);
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Schema *schema = session->schema();
  rime::Context *ctx = session->context();
  if (!schema || !ctx)
    return False;
  status->schema_id = new char[schema->schema_id().length() + 1];
  std::strcpy(status->schema_id, schema->schema_id().c_str());
  status->schema_name = new char[schema->schema_name().length() + 1];
  std::strcpy(status->schema_name, schema->schema_name().c_str());
  status->is_disabled = rime::Service::instance().disabled();
  status->is_composing = Bool(ctx->IsComposing());
  status->is_ascii_mode = Bool(ctx->get_option("ascii_mode"));
  status->is_full_shape = Bool(ctx->get_option("full_shape"));
  status->is_simplified = Bool(ctx->get_option("simplification"));
  status->is_traditional = Bool(ctx->get_option("traditional"));
  status->is_ascii_punct = Bool(ctx->get_option("ascii_punct"));
  return True;
}

RIME_API Bool RimeFreeStatus(RimeStatus* status) {
  if (!status || status->data_size <= 0)
    return False;
  delete[] status->schema_id;
  delete[] status->schema_name;
  RIME_STRUCT_CLEAR(*status);
  return True;
}

// runtime options

RIME_API void RimeSetOption(RimeSessionId session_id, const char* option, Bool value) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return;
  rime::Context *ctx = session->context();
  if (!ctx)
    return;
  ctx->set_option(option, !!value);
}

RIME_API Bool RimeGetOption(RimeSessionId session_id, const char* option) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Context *ctx = session->context();
  if (!ctx)
    return False;
  return Bool(ctx->get_option(option));
}

RIME_API void RimeSetProperty(RimeSessionId session_id, const char* prop, const char* value) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return;
  rime::Context *ctx = session->context();
  if (!ctx)
    return;
  ctx->set_property(prop, value);
}

RIME_API Bool RimeGetProperty(RimeSessionId session_id, const char* prop,
                              char* value, size_t buffer_size) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Context *ctx = session->context();
  if (!ctx)
    return False;
  std::string str_value(ctx->get_property(prop));
  if (str_value.empty())
    return False;
  strncpy(value, str_value.c_str(), buffer_size);
  return True;
}

RIME_API Bool RimeGetSchemaList(RimeSchemaList* output) {
  if (!output) return False;
  output->size = 0;
  output->list = NULL;
  rime::Schema default_schema;
  rime::Config* config = default_schema.config();
  if (!config) return False;
  rime::ConfigListPtr schema_list = config->GetList("schema_list");
  if (!schema_list || schema_list->size() == 0)
    return False;
  output->list = new RimeSchemaListItem[schema_list->size()];
  for (size_t i = 0; i < schema_list->size(); ++i) {
    rime::ConfigMapPtr item = rime::As<rime::ConfigMap>(schema_list->GetAt(i));
    if (!item) continue;
    rime::ConfigValuePtr schema_property = item->GetValue("schema");
    if (!schema_property) continue;
    const std::string &schema_id(schema_property->str());
    RimeSchemaListItem& x(output->list[output->size]);
    x.schema_id = new char[schema_id.length() + 1];
    strcpy(x.schema_id, schema_id.c_str());
    rime::Schema schema(schema_id);
    x.name = new char[schema.schema_name().length() + 1];
    strcpy(x.name, schema.schema_name().c_str());
    x.reserved = NULL;
    ++output->size;
  }
  if (output->size == 0) {
    delete[] output->list;
    output->list = NULL;
    return False;
  }
  return True;
}

RIME_API void RimeFreeSchemaList(RimeSchemaList* schema_list) {
  if (!schema_list) return;
  if (schema_list->list) {
    for (size_t i = 0; i < schema_list->size; ++i) {
      delete[] schema_list->list[i].schema_id;
      delete[] schema_list->list[i].name;
    }
    delete[] schema_list->list;
  }
  schema_list->size = 0;
  schema_list->list = NULL;
}

RIME_API Bool RimeGetCurrentSchema(RimeSessionId session_id, char* schema_id, size_t buffer_size) {
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session) return False;
  rime::Schema* schema = session->schema();
  if (!schema) return False;
  strncpy(schema_id, schema->schema_id().c_str(), buffer_size);
  return True;
}

RIME_API Bool RimeSelectSchema(RimeSessionId session_id, const char* schema_id) {
  if (!schema_id) return False;
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session) return False;
  session->ApplySchema(new rime::Schema(schema_id));
  return True;
}

// config

RIME_API Bool RimeSchemaOpen(const char *schema_id, RimeConfig* config) {
  if (!schema_id || !config) return False;
  rime::Config::Component* cc = rime::Config::Require("schema_config");
  if (!cc) return False;
  rime::Config* c = cc->Create(schema_id);
  if (!c) return False;
  config->ptr = (void*)c;
  return True;
}

RIME_API Bool RimeConfigOpen(const char *config_id, RimeConfig* config) {
  if (!config_id || !config) return False;
  rime::Config::Component* cc = rime::Config::Require("config");
  if (!cc) return False;
  rime::Config* c = cc->Create(config_id);
  if (!c) return False;
  config->ptr = (void*)c;
  return True;
}

RIME_API Bool RimeConfigClose(RimeConfig *config) {
  if (!config || !config->ptr) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  delete c;
  config->ptr = NULL;
  return True;
}

RIME_API Bool RimeConfigGetBool(RimeConfig *config, const char *key, Bool *value) {
  if (!config || !key || !value) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  bool bool_value = false;
  if (c->GetBool(key, &bool_value)) {
    *value = Bool(bool_value);
    return True;
  }
  return False;
}

RIME_API Bool RimeConfigGetInt(RimeConfig *config, const char *key, int *value) {
  if (!config || !key || !value) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  return Bool(c->GetInt(key, value));
}

RIME_API Bool RimeConfigGetDouble(RimeConfig *config, const char *key, double *value) {
  if (!config || !key || !value) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  return Bool(c->GetDouble(key, value));
}

RIME_API Bool RimeConfigGetString(RimeConfig *config, const char *key,
                                  char *value, size_t buffer_size) {
  if (!config || !key || !value) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c) return False;
  std::string str_value;
  if (c->GetString(key, &str_value)) {
    std::strncpy(value, str_value.c_str(), buffer_size);
    return True;
  }
  return False;
}

RIME_API const char* RimeConfigGetCString(RimeConfig *config, const char *key) {
  if (!config || !key) return NULL;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c) return NULL;
  if (rime::ConfigValuePtr v = c->GetValue(key)) {
    return v->str().c_str();
  }
  return NULL;
}

RIME_API Bool RimeConfigUpdateSignature(RimeConfig *config, const char* signer) {
  if (!config || !signer) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::Signature sig(signer);
  return Bool(sig.Sign(c, &deployer));
}

template <class T>
struct RimeConfigIteratorImpl {
  typename T::Iterator iter;
  typename T::Iterator end;
  std::string prefix;
  std::string key;
  std::string path;
  RimeConfigIteratorImpl<T>(T& container, const std::string& root_path)
      : iter(container.begin()),
        end(container.end()) {
    if (root_path.empty() || root_path == "/") {
      // prefix is empty
    }
    else {
      prefix = root_path + "/";
    }
  }
};

RIME_API Bool RimeConfigBeginList(RimeConfigIterator* iterator,
                                  RimeConfig* config, const char* key) {
  if (!iterator || !config || !key)
    return False;
  iterator->list = NULL;
  iterator->map = NULL;
  iterator->index = -1;
  iterator->key = NULL;
  iterator->path = NULL;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  rime::ConfigListPtr list = c->GetList(key);
  if (!list)
    return False;
  iterator->list = new RimeConfigIteratorImpl<rime::ConfigList>(*list, key);
  return True;
}

RIME_API Bool RimeConfigBeginMap(RimeConfigIterator* iterator,
                                 RimeConfig* config, const char* key) {
  if (!iterator || !config || !key)
    return False;
  iterator->list = NULL;
  iterator->map = NULL;
  iterator->index = -1;
  iterator->key = NULL;
  iterator->path = NULL;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c) return False;
  rime::ConfigMapPtr m = c->GetMap(key);
  if (!m) return False;
  iterator->map = new RimeConfigIteratorImpl<rime::ConfigMap>(*m, key);
  return True;
}

RIME_API Bool RimeConfigNext(RimeConfigIterator* iterator) {
  if (!iterator->list && !iterator->map)
    return False;
  if (iterator->list) {
    RimeConfigIteratorImpl<rime::ConfigList>* p =
        reinterpret_cast<RimeConfigIteratorImpl<rime::ConfigList>*>(iterator->list);
    if (!p) return False;
    if (++iterator->index > 0)
      ++p->iter;
    if (p->iter == p->end)
      return False;
    p->key = boost::str(boost::format("@%1%") % iterator->index);
    p->path = p->prefix + p->key;
    iterator->key = p->key.c_str();
    iterator->path = p->path.c_str();
    return True;
  }
  if (iterator->map) {
    RimeConfigIteratorImpl<rime::ConfigMap>* p =
        reinterpret_cast<RimeConfigIteratorImpl<rime::ConfigMap>*>(iterator->map);
    if (!p) return False;
    if (++iterator->index > 0)
      ++p->iter;
    if (p->iter == p->end)
      return False;
    p->key = p->iter->first;
    p->path = p->prefix + p->key;
    iterator->key = p->key.c_str();
    iterator->path = p->path.c_str();
    return True;
  }
  return False;
}

RIME_API void RimeConfigEnd(RimeConfigIterator* iterator) {
  if (!iterator) return;
  if (iterator->list)
    delete reinterpret_cast<RimeConfigIteratorImpl<rime::ConfigList>*>(iterator->list);
  if (iterator->map)
    delete reinterpret_cast<RimeConfigIteratorImpl<rime::ConfigMap>*>(iterator->map);
  memset(iterator, 0, sizeof(RimeConfigIterator));
}


RIME_API Bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence) {
  LOG(INFO) << "simulate key sequence: " << key_sequence;
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::KeySequence keys;
  if (!keys.Parse(key_sequence)) {
    LOG(ERROR) << "error parsing input: '" << key_sequence << "'";
    return False;
  }
  BOOST_FOREACH(const rime::KeyEvent& key, keys) {
    session->ProcessKey(key);
  }
  return True;
}

RIME_API Bool RimeRegisterModule(RimeModule* module) {
  if (!module || !module->module_name)
    return False;
  rime::ModuleManager::instance().Register(module->module_name, module);
  return True;
}

RIME_API RimeModule* RimeFindModule(const char* module_name) {
  return rime::ModuleManager::instance().Find(module_name);
}

RIME_API Bool RimeRunTask(const char* task_name) {
  if (!task_name)
    return False;
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return Bool(deployer.RunTask(task_name));
}

RIME_API const char* RimeGetSharedDataDir() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return deployer.shared_data_dir.c_str();
}

RIME_API const char* RimeGetUserDataDir() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return deployer.user_data_dir.c_str();
}

RIME_API const char* RimeGetSyncDir() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return deployer.sync_dir.c_str();
}

RIME_API const char* RimeGetUserId() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return deployer.user_id.c_str();
}

RIME_API void RimeGetUserDataSyncDir(char* dir, size_t buffer_size) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  strncpy(dir, deployer.user_data_sync_dir().c_str(), buffer_size);
}

RIME_API Bool RimeConfigInit(RimeConfig* config) {
  if (!config || config->ptr)
    return False;
  config->ptr = (void*)new rime::Config;
  return True;
}

RIME_API Bool RimeConfigLoadString(RimeConfig* config, const char* yaml) {
  if (!config || !yaml) {
    return False;
  }
  if (!config->ptr) {
    RimeConfigInit(config);
  }
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  std::istringstream iss(yaml);
  return Bool(c->LoadFromStream(iss));
}

RIME_API Bool RimeConfigGetItem(RimeConfig* config, const char* key, RimeConfig* value) {
  if (!config || !key || !value)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  if (!value->ptr) {
    RimeConfigInit(value);
  }
  rime::Config* v = reinterpret_cast<rime::Config*>(value->ptr);
  *v = c->GetItem(key);
  return True;
}

RIME_API Bool RimeConfigSetItem(RimeConfig* config, const char* key, RimeConfig* value) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  rime::ConfigItemPtr item;
  if (value) {
    if (rime::Config* v = reinterpret_cast<rime::Config*>(value->ptr)) {
      item = v->GetItem("");
    }
  }
  return Bool(c->SetItem(key, item));
}

RIME_API Bool RimeConfigSetBool(RimeConfig* config, const char* key, Bool value) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetBool(key, value));
}

RIME_API Bool RimeConfigSetInt(RimeConfig* config, const char* key, int value) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetInt(key, value));
}

RIME_API Bool RimeConfigSetDouble(RimeConfig* config, const char* key, double value) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetDouble(key, value));
}

RIME_API Bool RimeConfigSetString(RimeConfig* config, const char* key, const char* value) {
  if (!config || !key || !value)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetString(key, value));
}

RIME_API Bool RimeConfigClear(RimeConfig* config, const char* key) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetItem(key, rime::ConfigItemPtr()));
}

RIME_API Bool RimeConfigCreateList(RimeConfig* config, const char* key) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetItem(key, rime::New<rime::ConfigList>()));
}

RIME_API Bool RimeConfigCreateMap(RimeConfig* config, const char* key) {
  if (!config || !key)
    return False;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetItem(key, rime::New<rime::ConfigMap>()));
}

RIME_API size_t RimeConfigListSize(RimeConfig* config, const char* key) {
  if (!config || !key)
    return 0;
  rime::Config* c = reinterpret_cast<rime::Config*>(config->ptr);
  if (!c)
    return 0;
  if (rime::ConfigListPtr list = c->GetList(key)) {
    return list->size();
  }
  return 0;
}

const char* RimeGetInput(RimeSessionId session_id) {
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return NULL;
  rime::Context *ctx = session->context();
  if (!ctx)
    return NULL;
  return ctx->input().c_str();
}

size_t RimeGetCaretPos(RimeSessionId session_id) {
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return 0;
  rime::Context *ctx = session->context();
  if (!ctx)
    return 0;
  return ctx->caret_pos();
}

Bool RimeSelectCandidate(RimeSessionId session_id, size_t index) {
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Context *ctx = session->context();
  if (!ctx)
    return False;
  return Bool(ctx->Select(index));
}

RIME_API RimeApi* rime_get_api() {
  static RimeApi s_api = {0};
  if (!s_api.data_size) {
    RIME_STRUCT_INIT(RimeApi, s_api);
    s_api.setup = &RimeSetup;
    s_api.set_notification_handler = &RimeSetNotificationHandler;
    s_api.initialize = &RimeInitialize;
    s_api.finalize = &RimeFinalize;
    s_api.start_maintenance = &RimeStartMaintenance;
    s_api.is_maintenance_mode = &RimeIsMaintenancing;
    s_api.join_maintenance_thread = &RimeJoinMaintenanceThread;
    s_api.deployer_initialize = &RimeDeployerInitialize;
    s_api.prebuild = &RimePrebuildAllSchemas;
    s_api.deploy = &RimeDeployWorkspace;
    s_api.deploy_schema = &RimeDeploySchema;
    s_api.deploy_config_file = &RimeDeployConfigFile;
    s_api.sync_user_data = &RimeSyncUserData;
    s_api.create_session = &RimeCreateSession;
    s_api.find_session = &RimeFindSession;
    s_api.destroy_session = &RimeDestroySession;
    s_api.cleanup_stale_sessions = &RimeCleanupStaleSessions;
    s_api.cleanup_all_sessions = &RimeCleanupAllSessions;
    s_api.process_key = &RimeProcessKey;
    s_api.commit_composition = &RimeCommitComposition;
    s_api.clear_composition = &RimeClearComposition;
    s_api.get_commit = &RimeGetCommit;
    s_api.free_commit = &RimeFreeCommit;
    s_api.get_context = &RimeGetContext;
    s_api.free_context = &RimeFreeContext;
    s_api.get_status =  &RimeGetStatus;
    s_api.free_status = &RimeFreeStatus;
    s_api.set_option = &RimeSetOption;
    s_api.get_option = &RimeGetOption;
    s_api.set_property = &RimeSetProperty;
    s_api.get_property = &RimeGetProperty;
    s_api.get_schema_list = &RimeGetSchemaList;
    s_api.free_schema_list = &RimeFreeSchemaList;
    s_api.get_current_schema = &RimeGetCurrentSchema;
    s_api.select_schema = &RimeSelectSchema;
    s_api.schema_open = &RimeSchemaOpen;
    s_api.config_open = &RimeConfigOpen;
    s_api.config_close = &RimeConfigClose;
    s_api.config_get_bool = &RimeConfigGetBool;
    s_api.config_get_int = &RimeConfigGetInt;
    s_api.config_get_double = &RimeConfigGetDouble;
    s_api.config_get_string = &RimeConfigGetString;
    s_api.config_get_cstring = &RimeConfigGetCString;
    s_api.config_update_signature = &RimeConfigUpdateSignature;
    s_api.config_begin_map = &RimeConfigBeginMap;
    s_api.config_next = &RimeConfigNext;
    s_api.config_end = &RimeConfigEnd;
    s_api.simulate_key_sequence = &RimeSimulateKeySequence;
    s_api.register_module = &RimeRegisterModule;
    s_api.find_module = &RimeFindModule;
    s_api.run_task = &RimeRunTask;
    s_api.get_shared_data_dir = &RimeGetSharedDataDir;
    s_api.get_user_data_dir = &RimeGetUserDataDir;
    s_api.get_sync_dir = &RimeGetSyncDir;
    s_api.get_user_id = &RimeGetUserId;
    s_api.get_user_data_sync_dir = &RimeGetUserDataSyncDir;
    s_api.config_init = &RimeConfigInit;
    s_api.config_load_string = &RimeConfigLoadString;
    s_api.config_set_bool = &RimeConfigSetBool;
    s_api.config_set_int = &RimeConfigSetInt;
    s_api.config_set_double = &RimeConfigSetDouble;
    s_api.config_set_string = &RimeConfigSetString;
    s_api.config_get_item = &RimeConfigGetItem;
    s_api.config_set_item = &RimeConfigSetItem;
    s_api.config_clear = &RimeConfigClear;
    s_api.config_create_list = &RimeConfigCreateList;
    s_api.config_create_map = &RimeConfigCreateMap;
    s_api.config_list_size = &RimeConfigListSize;
    s_api.config_begin_list = &RimeConfigBeginList;
    s_api.get_input = &RimeGetInput;
    s_api.get_caret_pos = &RimeGetCaretPos;
    s_api.select_candidate = &RimeSelectCandidate;
  }
  return &s_api;
}
