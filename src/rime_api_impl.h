// intended to be included multiple times in c++ source files with different
// RIME_FLAVORED macro definitions

#include "rime_api.h"

#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
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
#include <rime/switches.h>

using namespace rime;

void rime_declare_module_dependencies();

RIME_DEPRECATED void RimeSetup(RimeTraits* traits) {
  rime_declare_module_dependencies();

  SetupDeployer(traits);
  if (RIME_PROVIDED(traits, app_name)) {
    if (RIME_STRUCT_HAS_MEMBER(*traits, traits->min_log_level) &&
        RIME_STRUCT_HAS_MEMBER(*traits, traits->log_dir)) {
      SetupLogging(traits->app_name, traits->min_log_level, traits->log_dir);
    } else {
      SetupLogging(traits->app_name);
    }
  }
}

RIME_DEPRECATED void RimeSetNotificationHandler(RimeNotificationHandler handler,
                                                void* context_object) {
  if (handler) {
    Service::instance().SetNotificationHandler(
        [context_object, handler](auto id, auto type, auto value) {
          handler(context_object, id, type, value);
        });
  } else {
    Service::instance().ClearNotificationHandler();
  }
}

RIME_DEPRECATED void RimeInitialize(RimeTraits* traits) {
  SetupDeployer(traits);
  LoadModules(RIME_PROVIDED(traits, modules) ? traits->modules
                                             : kDefaultModules);
  Service::instance().StartService();
}

RIME_DEPRECATED void RimeFinalize() {
  Service::instance().deployer().JoinMaintenanceThread();
  Service::instance().StopService();
  Registry::instance().Clear();
  ModuleManager::instance().UnloadModules();
}

RIME_DEPRECATED Bool RimeStartMaintenance(Bool full_check) {
  LoadModules(kDeployerModules);
  Deployer& deployer(Service::instance().deployer());
  deployer.RunTask("clean_old_log_files");
  if (!deployer.RunTask("installation_update")) {
    return False;
  }
  if (!full_check) {
    TaskInitializer args{
        vector<path>{
            deployer.user_data_dir,
            deployer.shared_data_dir,
        },
    };
    if (!deployer.RunTask("detect_modifications", args)) {
      return False;
    }
    LOG(INFO) << "changes detected; starting maintenance.";
  }
  deployer.ScheduleTask("workspace_update");
  deployer.ScheduleTask("user_dict_upgrade");
  deployer.ScheduleTask("cleanup_trash");
  deployer.StartMaintenance();
  return True;
}

RIME_DEPRECATED Bool RimeStartMaintenanceOnWorkspaceChange() {
  return RimeStartMaintenance(False);
}

RIME_DEPRECATED Bool RimeIsMaintenancing() {
  Deployer& deployer(Service::instance().deployer());
  return Bool(deployer.IsMaintenanceMode());
}

RIME_DEPRECATED void RimeJoinMaintenanceThread() {
  Deployer& deployer(Service::instance().deployer());
  deployer.JoinMaintenanceThread();
}

// deployment

RIME_DEPRECATED void RimeDeployerInitialize(RimeTraits* traits) {
  SetupDeployer(traits);
  LoadModules(RIME_PROVIDED(traits, modules) ? traits->modules
                                             : kDeployerModules);
}

RIME_DEPRECATED Bool RimePrebuildAllSchemas() {
  Deployer& deployer(Service::instance().deployer());
  return Bool(deployer.RunTask("prebuild_all_schemas"));
}

RIME_DEPRECATED Bool RimeDeployWorkspace() {
  Deployer& deployer(Service::instance().deployer());
  return Bool(deployer.RunTask("installation_update") &&
              deployer.RunTask("workspace_update") &&
              deployer.RunTask("user_dict_upgrade") &&
              deployer.RunTask("cleanup_trash"));
}

RIME_DEPRECATED Bool RimeDeploySchema(const char* schema_file) {
  Deployer& deployer(Service::instance().deployer());
  return Bool(deployer.RunTask("schema_update", path(schema_file)));
}

RIME_DEPRECATED Bool RimeDeployConfigFile(const char* file_name,
                                          const char* version_key) {
  Deployer& deployer(Service::instance().deployer());
  TaskInitializer args(make_pair<string, string>(file_name, version_key));
  return Bool(deployer.RunTask("config_file_update", args));
}

RIME_DEPRECATED Bool RimeSyncUserData() {
  Service::instance().CleanupAllSessions();
  Deployer& deployer(Service::instance().deployer());
  deployer.ScheduleTask("installation_update");
  deployer.ScheduleTask("backup_config_files");
  deployer.ScheduleTask("user_dict_sync");
  return Bool(deployer.StartMaintenance());
}

// session management

RIME_DEPRECATED RimeSessionId RimeCreateSession() {
  return Service::instance().CreateSession();
}

RIME_DEPRECATED Bool RimeFindSession(RimeSessionId session_id) {
  return Bool(session_id && Service::instance().GetSession(session_id));
}

RIME_DEPRECATED Bool RimeDestroySession(RimeSessionId session_id) {
  return Bool(Service::instance().DestroySession(session_id));
}

RIME_DEPRECATED void RimeCleanupStaleSessions() {
  Service::instance().CleanupStaleSessions();
}

RIME_DEPRECATED void RimeCleanupAllSessions() {
  Service::instance().CleanupAllSessions();
}

// input

RIME_DEPRECATED Bool RimeProcessKey(RimeSessionId session_id,
                                    int keycode,
                                    int mask) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  return Bool(session->ProcessKey(KeyEvent(keycode, mask)));
}

RIME_DEPRECATED Bool RimeCommitComposition(RimeSessionId session_id) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  return Bool(session->CommitComposition());
}

RIME_DEPRECATED void RimeClearComposition(RimeSessionId session_id) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return;
  session->ClearComposition();
}

// output

static void rime_candidate_copy(RimeCandidate* dest, const an<Candidate>& src) {
  dest->text = new char[src->text().length() + 1];
  std::strcpy(dest->text, src->text().c_str());
  string comment(src->comment());
  if (!comment.empty()) {
    dest->comment = new char[comment.length() + 1];
    std::strcpy(dest->comment, comment.c_str());
  } else {
    dest->comment = nullptr;
  }
  dest->reserved = nullptr;
}

RIME_DEPRECATED Bool RimeGetContext(RimeSessionId session_id,
                                    RIME_FLAVORED(RimeContext) * context) {
  if (!context || context->data_size <= 0)
    return False;
  RIME_STRUCT_CLEAR(*context);
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Context* ctx = session->context();
  if (!ctx)
    return False;
  if (ctx->IsComposing()) {
    Preedit preedit = ctx->GetPreedit();
    context->composition.length = preedit.text.length();
    context->composition.preedit = new char[preedit.text.length() + 1];
    std::strcpy(context->composition.preedit, preedit.text.c_str());
    context->composition.cursor_pos = preedit.caret_pos;
    context->composition.sel_start = preedit.sel_start;
    context->composition.sel_end = preedit.sel_end;
    if (RIME_STRUCT_HAS_MEMBER(*context, context->commit_text_preview)) {
      string commit_text(ctx->GetCommitText());
      if (!commit_text.empty()) {
        context->commit_text_preview = new char[commit_text.length() + 1];
        std::strcpy(context->commit_text_preview, commit_text.c_str());
      }
    }
  }
  if (ctx->HasMenu()) {
    Segment& seg(ctx->composition().back());
    int page_size = 5;
    Schema* schema = session->schema();
    if (schema)
      page_size = schema->page_size();
    int selected_index = seg.selected_index;
    int page_no = selected_index / page_size;
    the<Page> page(seg.menu->CreatePage(page_size, page_no));
    if (page) {
      context->menu.page_size = page_size;
      context->menu.page_no = page_no;
      context->menu.is_last_page = Bool(page->is_last_page);
      context->menu.highlighted_candidate_index = selected_index % page_size;
      int i = 0;
      context->menu.num_candidates = page->candidates.size();
      context->menu.candidates = new RimeCandidate[page->candidates.size()];
      for (const an<Candidate>& cand : page->candidates) {
        RimeCandidate* dest = &context->menu.candidates[i++];
        rime_candidate_copy(dest, cand);
      }
      if (schema) {
        const string& select_keys(schema->select_keys());
        if (!select_keys.empty()) {
          context->menu.select_keys = new char[select_keys.length() + 1];
          std::strcpy(context->menu.select_keys, select_keys.c_str());
        }
        Config* config = schema->config();
        an<ConfigList> select_labels =
            config->GetList("menu/alternative_select_labels");
        if (select_labels && (size_t)page_size <= select_labels->size()) {
          context->select_labels = new char*[page_size];
          for (size_t i = 0; i < (size_t)page_size; ++i) {
            an<ConfigValue> value = select_labels->GetValueAt(i);
            string label = value->str();
            context->select_labels[i] = new char[label.length() + 1];
            std::strcpy(context->select_labels[i], label.c_str());
          }
        }
      }
    }
  }
  return True;
}

RIME_DEPRECATED Bool RimeFreeContext(RIME_FLAVORED(RimeContext) * context) {
  if (!context || context->data_size <= 0)
    return False;
  delete[] context->composition.preedit;
  for (int i = 0; i < context->menu.num_candidates; ++i) {
    delete[] context->menu.candidates[i].text;
    delete[] context->menu.candidates[i].comment;
  }
  delete[] context->menu.candidates;
  delete[] context->menu.select_keys;
  if (RIME_STRUCT_HAS_MEMBER(*context, context->select_labels) &&
      context->select_labels) {
    for (int i = 0; i < context->menu.page_size; ++i) {
      delete[] context->select_labels[i];
    }
    delete[] context->select_labels;
  }
  if (RIME_STRUCT_HAS_MEMBER(*context, context->commit_text_preview)) {
    delete[] context->commit_text_preview;
  }
  RIME_STRUCT_CLEAR(*context);
  return True;
}

RIME_DEPRECATED Bool RimeGetCommit(RimeSessionId session_id,
                                   RimeCommit* commit) {
  if (!commit)
    return False;
  RIME_STRUCT_CLEAR(*commit);
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  const string& commit_text(session->commit_text());
  if (!commit_text.empty()) {
    commit->text = new char[commit_text.length() + 1];
    std::strcpy(commit->text, commit_text.c_str());
    session->ResetCommitText();
    return True;
  }
  return False;
}

RIME_DEPRECATED Bool RimeFreeCommit(RimeCommit* commit) {
  if (!commit)
    return False;
  delete[] commit->text;
  RIME_STRUCT_CLEAR(*commit);
  return True;
}

RIME_DEPRECATED Bool RimeGetStatus(RimeSessionId session_id,
                                   RIME_FLAVORED(RimeStatus) * status) {
  if (!status || status->data_size <= 0)
    return False;
  RIME_STRUCT_CLEAR(*status);
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Schema* schema = session->schema();
  Context* ctx = session->context();
  if (!schema || !ctx)
    return False;
  status->schema_id = new char[schema->schema_id().length() + 1];
  std::strcpy(status->schema_id, schema->schema_id().c_str());
  status->schema_name = new char[schema->schema_name().length() + 1];
  std::strcpy(status->schema_name, schema->schema_name().c_str());
  status->is_disabled = Bool(Service::instance().disabled());
  status->is_composing = Bool(ctx->IsComposing());
  status->is_ascii_mode = Bool(ctx->get_option("ascii_mode"));
  status->is_full_shape = Bool(ctx->get_option("full_shape"));
  status->is_simplified = Bool(ctx->get_option("simplification"));
  status->is_traditional = Bool(ctx->get_option("traditional"));
  status->is_ascii_punct = Bool(ctx->get_option("ascii_punct"));
  return True;
}

RIME_DEPRECATED Bool RimeFreeStatus(RIME_FLAVORED(RimeStatus) * status) {
  if (!status || status->data_size <= 0)
    return False;
  delete[] status->schema_id;
  delete[] status->schema_name;
  RIME_STRUCT_CLEAR(*status);
  return True;
}

// accessing candidate list

RIME_DEPRECATED Bool
RimeCandidateListFromIndex(RimeSessionId session_id,
                           RimeCandidateListIterator* iterator,
                           int index) {
  if (!iterator)
    return False;
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Context* ctx = session->context();
  if (!ctx || !ctx->HasMenu())
    return False;
  memset(iterator, 0, sizeof(RimeCandidateListIterator));
  iterator->ptr = ctx->composition().back().menu.get();
  iterator->index = index - 1;
  return True;
}

RIME_DEPRECATED Bool
RimeCandidateListBegin(RimeSessionId session_id,
                       RimeCandidateListIterator* iterator) {
  return RimeCandidateListFromIndex(session_id, iterator, 0);
}

RIME_DEPRECATED Bool
RimeCandidateListNext(RimeCandidateListIterator* iterator) {
  if (!iterator)
    return False;
  Menu* menu = reinterpret_cast<Menu*>(iterator->ptr);
  if (!menu)
    return False;
  ++iterator->index;
  if (auto cand = menu->GetCandidateAt((size_t)iterator->index)) {
    delete[] iterator->candidate.text;
    delete[] iterator->candidate.comment;
    rime_candidate_copy(&iterator->candidate, cand);
    return True;
  }
  return False;
}

RIME_DEPRECATED void RimeCandidateListEnd(RimeCandidateListIterator* iterator) {
  if (!iterator)
    return;
  delete[] iterator->candidate.text;
  delete[] iterator->candidate.comment;
  memset(iterator, 0, sizeof(RimeCandidateListIterator));
}

// runtime options

RIME_DEPRECATED void RimeSetOption(RimeSessionId session_id,
                                   const char* option,
                                   Bool value) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return;
  Context* ctx = session->context();
  if (!ctx)
    return;
  ctx->set_option(option, !!value);
}

RIME_DEPRECATED Bool RimeGetOption(RimeSessionId session_id,
                                   const char* option) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Context* ctx = session->context();
  if (!ctx)
    return False;
  return Bool(ctx->get_option(option));
}

RIME_DEPRECATED void RimeSetProperty(RimeSessionId session_id,
                                     const char* prop,
                                     const char* value) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return;
  Context* ctx = session->context();
  if (!ctx)
    return;
  ctx->set_property(prop, value);
}

RIME_DEPRECATED Bool RimeGetProperty(RimeSessionId session_id,
                                     const char* prop,
                                     char* value,
                                     size_t buffer_size) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Context* ctx = session->context();
  if (!ctx)
    return False;
  string str_value(ctx->get_property(prop));
  if (str_value.empty())
    return False;
  strncpy(value, str_value.c_str(), buffer_size);
  return True;
}

RIME_DEPRECATED Bool RimeGetSchemaList(RimeSchemaList* output) {
  if (!output)
    return False;
  output->size = 0;
  output->list = NULL;
  Schema default_schema;
  Config* config = default_schema.config();
  if (!config)
    return False;
  an<ConfigList> schema_list = config->GetList("schema_list");
  if (!schema_list || schema_list->size() == 0)
    return False;
  output->list = new RimeSchemaListItem[schema_list->size()];
  for (size_t i = 0; i < schema_list->size(); ++i) {
    an<ConfigMap> item = As<ConfigMap>(schema_list->GetAt(i));
    if (!item)
      continue;
    an<ConfigValue> schema_property = item->GetValue("schema");
    if (!schema_property)
      continue;
    const string& schema_id(schema_property->str());
    RimeSchemaListItem& x(output->list[output->size]);
    x.schema_id = new char[schema_id.length() + 1];
    strcpy(x.schema_id, schema_id.c_str());
    Schema schema(schema_id);
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

RIME_DEPRECATED void RimeFreeSchemaList(RimeSchemaList* schema_list) {
  if (!schema_list)
    return;
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

RIME_DEPRECATED Bool RimeGetCurrentSchema(RimeSessionId session_id,
                                          char* schema_id,
                                          size_t buffer_size) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Schema* schema = session->schema();
  if (!schema)
    return False;
  strncpy(schema_id, schema->schema_id().c_str(), buffer_size);
  return True;
}

RIME_DEPRECATED Bool RimeSelectSchema(RimeSessionId session_id,
                                      const char* schema_id) {
  if (!schema_id)
    return False;
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  session->ApplySchema(new Schema(schema_id));
  return True;
}

// config

static Bool open_config_in_component(const char* config_component,
                                     const char* config_id,
                                     RimeConfig* config) {
  if (!config_id || !config)
    return False;
  Config::Component* cc = Config::Require(config_component);
  if (!cc)
    return False;
  Config* c = cc->Create(config_id);
  if (!c)
    return False;
  config->ptr = (void*)c;
  return True;
}

RIME_DEPRECATED Bool RimeSchemaOpen(const char* schema_id, RimeConfig* config) {
  return open_config_in_component("schema", schema_id, config);
}

RIME_DEPRECATED Bool RimeConfigOpen(const char* config_id, RimeConfig* config) {
  return open_config_in_component("config", config_id, config);
}

RIME_DEPRECATED Bool RimeUserConfigOpen(const char* config_id,
                                        RimeConfig* config) {
  return open_config_in_component("user_config", config_id, config);
}

RIME_DEPRECATED Bool RimeConfigClose(RimeConfig* config) {
  if (!config || !config->ptr)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  delete c;
  config->ptr = NULL;
  return True;
}

RIME_DEPRECATED Bool RimeConfigGetBool(RimeConfig* config,
                                       const char* key,
                                       Bool* value) {
  if (!config || !key || !value)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  bool bool_value = false;
  if (c->GetBool(key, &bool_value)) {
    *value = Bool(bool_value);
    return True;
  }
  return False;
}

RIME_DEPRECATED Bool RimeConfigGetInt(RimeConfig* config,
                                      const char* key,
                                      int* value) {
  if (!config || !key || !value)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  return Bool(c->GetInt(key, value));
}

RIME_DEPRECATED Bool RimeConfigGetDouble(RimeConfig* config,
                                         const char* key,
                                         double* value) {
  if (!config || !key || !value)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  return Bool(c->GetDouble(key, value));
}

RIME_DEPRECATED Bool RimeConfigGetString(RimeConfig* config,
                                         const char* key,
                                         char* value,
                                         size_t buffer_size) {
  if (!config || !key || !value)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  string str_value;
  if (c->GetString(key, &str_value)) {
    std::strncpy(value, str_value.c_str(), buffer_size);
    return True;
  }
  return False;
}

RIME_DEPRECATED const char* RimeConfigGetCString(RimeConfig* config,
                                                 const char* key) {
  if (!config || !key)
    return NULL;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return NULL;
  if (an<ConfigValue> v = c->GetValue(key)) {
    return v->str().c_str();
  }
  return NULL;
}

RIME_DEPRECATED Bool RimeConfigUpdateSignature(RimeConfig* config,
                                               const char* signer) {
  if (!config || !signer)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  Deployer& deployer(Service::instance().deployer());
  Signature sig(signer);
  return Bool(sig.Sign(c, &deployer));
}

template <class T>
struct RimeConfigIteratorImpl {
  typename T::Iterator iter;
  typename T::Iterator end;
  string prefix;
  string key;
  string path;
  RimeConfigIteratorImpl<T>(T& container, const string& root_path)
      : iter(container.begin()), end(container.end()) {
    if (root_path.empty() || root_path == "/") {
      // prefix is empty
    } else {
      prefix = root_path + "/";
    }
  }
};

RIME_DEPRECATED Bool RimeConfigBeginList(RimeConfigIterator* iterator,
                                         RimeConfig* config,
                                         const char* key) {
  if (!iterator || !config || !key)
    return False;
  iterator->list = NULL;
  iterator->map = NULL;
  iterator->index = -1;
  iterator->key = NULL;
  iterator->path = NULL;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  an<ConfigList> list = c->GetList(key);
  if (!list)
    return False;
  iterator->list = new RimeConfigIteratorImpl<ConfigList>(*list, key);
  return True;
}

RIME_DEPRECATED Bool RimeConfigBeginMap(RimeConfigIterator* iterator,
                                        RimeConfig* config,
                                        const char* key) {
  if (!iterator || !config || !key)
    return False;
  iterator->list = NULL;
  iterator->map = NULL;
  iterator->index = -1;
  iterator->key = NULL;
  iterator->path = NULL;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  an<ConfigMap> m = c->GetMap(key);
  if (!m)
    return False;
  iterator->map = new RimeConfigIteratorImpl<ConfigMap>(*m, key);
  return True;
}

RIME_DEPRECATED Bool RimeConfigNext(RimeConfigIterator* iterator) {
  if (!iterator->list && !iterator->map)
    return False;
  if (iterator->list) {
    RimeConfigIteratorImpl<ConfigList>* p =
        reinterpret_cast<RimeConfigIteratorImpl<ConfigList>*>(iterator->list);
    if (!p)
      return False;
    if (++iterator->index > 0)
      ++p->iter;
    if (p->iter == p->end)
      return False;
    std::ostringstream key;
    key << "@" << iterator->index;
    p->key = key.str();
    p->path = p->prefix + p->key;
    iterator->key = p->key.c_str();
    iterator->path = p->path.c_str();
    return True;
  }
  if (iterator->map) {
    RimeConfigIteratorImpl<ConfigMap>* p =
        reinterpret_cast<RimeConfigIteratorImpl<ConfigMap>*>(iterator->map);
    if (!p)
      return False;
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

RIME_DEPRECATED void RimeConfigEnd(RimeConfigIterator* iterator) {
  if (!iterator)
    return;
  if (iterator->list)
    delete reinterpret_cast<RimeConfigIteratorImpl<ConfigList>*>(
        iterator->list);
  if (iterator->map)
    delete reinterpret_cast<RimeConfigIteratorImpl<ConfigMap>*>(iterator->map);
  memset(iterator, 0, sizeof(RimeConfigIterator));
}

RIME_DEPRECATED Bool RimeSimulateKeySequence(RimeSessionId session_id,
                                             const char* key_sequence) {
  LOG(INFO) << "simulate key sequence: " << key_sequence;
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  KeySequence keys;
  if (!keys.Parse(key_sequence)) {
    LOG(ERROR) << "error parsing input: '" << key_sequence << "'";
    return False;
  }
  for (const KeyEvent& key : keys) {
    session->ProcessKey(key);
  }
  return True;
}

RIME_DEPRECATED Bool RimeRunTask(const char* task_name) {
  if (!task_name)
    return False;
  Deployer& deployer(Service::instance().deployer());
  return Bool(deployer.RunTask(task_name));
}

RIME_DEPRECATED const char* RimeGetSharedDataDir() {
  Deployer& deployer(Service::instance().deployer());
  static string string_path;
  string_path = deployer.shared_data_dir.string();
  return string_path.c_str();
}

RIME_DEPRECATED const char* RimeGetUserDataDir() {
  Deployer& deployer(Service::instance().deployer());
  static string string_path;
  string_path = deployer.user_data_dir.string();
  return string_path.c_str();
}

RIME_DEPRECATED const char* RimeGetPrebuiltDataDir() {
  Deployer& deployer(Service::instance().deployer());
  static string string_path;
  string_path = deployer.prebuilt_data_dir.string();
  return string_path.c_str();
}

RIME_DEPRECATED const char* RimeGetStagingDir() {
  Deployer& deployer(Service::instance().deployer());
  static string string_path;
  string_path = deployer.staging_dir.string();
  return string_path.c_str();
}

RIME_DEPRECATED const char* RimeGetSyncDir() {
  Deployer& deployer(Service::instance().deployer());
  static string string_path;
  string_path = deployer.sync_dir.string();
  return string_path.c_str();
}

RIME_DEPRECATED const char* RimeGetUserId() {
  Deployer& deployer(Service::instance().deployer());
  return deployer.user_id.c_str();
}

RIME_DEPRECATED void RimeGetUserDataSyncDir(char* dir, size_t buffer_size) {
  Deployer& deployer(Service::instance().deployer());
  string string_path = deployer.user_data_sync_dir().string();
  strncpy(dir, string_path.c_str(), buffer_size);
}

RIME_DEPRECATED Bool RimeConfigInit(RimeConfig* config) {
  if (!config || config->ptr)
    return False;
  config->ptr = (void*)new Config;
  return True;
}

RIME_DEPRECATED Bool RimeConfigLoadString(RimeConfig* config,
                                          const char* yaml) {
  if (!config || !yaml) {
    return False;
  }
  if (!config->ptr) {
    RimeConfigInit(config);
  }
  Config* c = reinterpret_cast<Config*>(config->ptr);
  std::istringstream iss(yaml);
  return Bool(c->LoadFromStream(iss));
}

RIME_DEPRECATED Bool RimeConfigGetItem(RimeConfig* config,
                                       const char* key,
                                       RimeConfig* value) {
  if (!config || !key || !value)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  if (!value->ptr) {
    RimeConfigInit(value);
  }
  Config* v = reinterpret_cast<Config*>(value->ptr);
  *v = c->GetItem(key);
  return True;
}

RIME_DEPRECATED Bool RimeConfigSetItem(RimeConfig* config,
                                       const char* key,
                                       RimeConfig* value) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  an<ConfigItem> item;
  if (value) {
    if (Config* v = reinterpret_cast<Config*>(value->ptr)) {
      item = v->GetItem("");
    }
  }
  return Bool(c->SetItem(key, item));
}

RIME_DEPRECATED Bool RimeConfigSetBool(RimeConfig* config,
                                       const char* key,
                                       Bool value) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return c->SetBool(key, value != False);
}

RIME_DEPRECATED Bool RimeConfigSetInt(RimeConfig* config,
                                      const char* key,
                                      int value) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetInt(key, value));
}

RIME_DEPRECATED Bool RimeConfigSetDouble(RimeConfig* config,
                                         const char* key,
                                         double value) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetDouble(key, value));
}

RIME_DEPRECATED Bool RimeConfigSetString(RimeConfig* config,
                                         const char* key,
                                         const char* value) {
  if (!config || !key || !value)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetString(key, value));
}

RIME_DEPRECATED Bool RimeConfigClear(RimeConfig* config, const char* key) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetItem(key, nullptr));
}

RIME_DEPRECATED Bool RimeConfigCreateList(RimeConfig* config, const char* key) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetItem(key, New<ConfigList>()));
}

RIME_DEPRECATED Bool RimeConfigCreateMap(RimeConfig* config, const char* key) {
  if (!config || !key)
    return False;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return False;
  return Bool(c->SetItem(key, New<ConfigMap>()));
}

RIME_DEPRECATED size_t RimeConfigListSize(RimeConfig* config, const char* key) {
  if (!config || !key)
    return 0;
  Config* c = reinterpret_cast<Config*>(config->ptr);
  if (!c)
    return 0;
  if (an<ConfigList> list = c->GetList(key)) {
    return list->size();
  }
  return 0;
}

static bool do_with_candidate(RimeSessionId session_id,
                              size_t index,
                              bool (Context::*verb)(size_t index)) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return false;
  Context* ctx = session->context();
  if (!ctx)
    return false;
  return (ctx->*verb)(index);
}

static bool do_with_candidate_on_current_page(
    RimeSessionId session_id,
    size_t index,
    bool (Context::*verb)(size_t index)) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return false;
  Context* ctx = session->context();
  if (!ctx || !ctx->HasMenu())
    return false;
  Schema* schema = session->schema();
  if (!schema)
    return false;
  size_t page_size = (size_t)schema->page_size();
  if (index >= page_size)
    return false;
  const auto& seg(ctx->composition().back());
  size_t page_start = seg.selected_index / page_size * page_size;
  return (ctx->*verb)(page_start + index);
}

static Bool RimeChangePage(RimeSessionId session_id, Bool backward) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Context* ctx = session->context();
  if (!ctx || !ctx->HasMenu())
    return False;
  Schema* schema = session->schema();
  if (!schema)
    return False;
  size_t page_size = (size_t)schema->page_size();
  auto& seg(ctx->composition().back());
  size_t current_index = seg.selected_index;
  size_t index =
      backward ? (current_index <= page_size ? 0 : current_index - page_size)
               : (current_index + page_size);
  DLOG(INFO) << "current selection: " << current_index << ", flipping "
             << (backward ? "backward" : "forward") << ", new selection "
             << index;
  seg.tags.insert("paging");
  return Bool(ctx->Highlight(index));
}

static Bool RimeHighlightCandidate(RimeSessionId session_id, size_t index) {
  return (Bool)do_with_candidate(session_id, index, &Context::Highlight);
}

static Bool RimeHighlightCandidateOnCurrentPage(RimeSessionId session_id,
                                                size_t index) {
  return (Bool)do_with_candidate_on_current_page(session_id, index,
                                                 &Context::Highlight);
}

RIME_DEPRECATED Bool RimeSelectCandidate(RimeSessionId session_id,
                                         size_t index) {
  return (Bool)do_with_candidate(session_id, index, &Context::Select);
}

RIME_DEPRECATED Bool RimeSelectCandidateOnCurrentPage(RimeSessionId session_id,
                                                      size_t index) {
  return (Bool)do_with_candidate_on_current_page(session_id, index,
                                                 &Context::Select);
}

RIME_DEPRECATED Bool RimeDeleteCandidate(RimeSessionId session_id,
                                         size_t index) {
  return (Bool)do_with_candidate(session_id, index, &Context::DeleteCandidate);
}

RIME_DEPRECATED Bool RimeDeleteCandidateOnCurrentPage(RimeSessionId session_id,
                                                      size_t index) {
  return (Bool)do_with_candidate_on_current_page(session_id, index,
                                                 &Context::DeleteCandidate);
}

static const char* RimeGetInput(RimeSessionId session_id) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return NULL;
  Context* ctx = session->context();
  if (!ctx)
    return NULL;
  return ctx->input().c_str();
}

RIME_DEPRECATED Bool RimeSetInput(RimeSessionId session_id, const char* input) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return False;
  Context* ctx = session->context();
  if (!ctx)
    return False;
  ctx->set_input(input);
  return True;
}

static size_t RimeGetCaretPos(RimeSessionId session_id) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return 0;
  Context* ctx = session->context();
  if (!ctx)
    return 0;
  return ctx->caret_pos();
}

static void RimeSetCaretPos(RimeSessionId session_id, size_t caret_pos) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return;
  Context* ctx = session->context();
  if (!ctx)
    return;
  return ctx->set_caret_pos(caret_pos);
}

static RimeStringSlice RimeGetStateLabelAbbreviated(RimeSessionId session_id,
                                                    const char* option_name,
                                                    Bool state,
                                                    Bool abbreviated) {
  an<Session> session(Service::instance().GetSession(session_id));
  if (!session)
    return {nullptr, 0};
  Config* config = session->schema()->config();
  if (!config)
    return {nullptr, 0};
  Switches switches(config);
  StringSlice label = switches.GetStateLabel(option_name, state, abbreviated);
  return {label.str, label.length};
}

static const char* RimeGetStateLabel(RimeSessionId session_id,
                                     const char* option_name,
                                     Bool state) {
  return RimeGetStateLabelAbbreviated(session_id, option_name, state, False)
      .str;
}

void RimeGetSharedDataDirSecure(char* dir, size_t buffer_size);
void RimeGetUserDataDirSecure(char* dir, size_t buffer_size);
void RimeGetPrebuiltDataDirSecure(char* dir, size_t buffer_size);
void RimeGetStagingDirSecure(char* dir, size_t buffer_size);
void RimeGetSyncDirSecure(char* dir, size_t buffer_size);
const char* RimeGetVersion();

RIME_API RIME_FLAVORED(RimeApi) * RIME_FLAVORED(rime_get_api)() {
  static RIME_FLAVORED(RimeApi) s_api = {0};
  if (!s_api.data_size) {
    RIME_STRUCT_INIT(RIME_FLAVORED(RimeApi), s_api);
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
    s_api.get_status = &RimeGetStatus;
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
    s_api.user_config_open = &RimeUserConfigOpen;
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
    s_api.get_version = &RimeGetVersion;
    s_api.set_caret_pos = &RimeSetCaretPos;
    s_api.select_candidate_on_current_page = &RimeSelectCandidateOnCurrentPage;
    s_api.candidate_list_begin = &RimeCandidateListBegin;
    s_api.candidate_list_next = &RimeCandidateListNext;
    s_api.candidate_list_end = &RimeCandidateListEnd;
    s_api.candidate_list_from_index = &RimeCandidateListFromIndex;
    s_api.get_prebuilt_data_dir = &RimeGetPrebuiltDataDir;
    s_api.get_staging_dir = &RimeGetStagingDir;
    s_api.commit_proto = nullptr;
    s_api.context_proto = nullptr;
    s_api.status_proto = nullptr;
    s_api.get_state_label = &RimeGetStateLabel;
    s_api.delete_candidate = &RimeDeleteCandidate;
    s_api.delete_candidate_on_current_page = &RimeDeleteCandidateOnCurrentPage;
    s_api.get_state_label_abbreviated = &RimeGetStateLabelAbbreviated;
    s_api.set_input = &RimeSetInput;
    s_api.get_shared_data_dir_s = &RimeGetSharedDataDirSecure;
    s_api.get_user_data_dir_s = &RimeGetUserDataDirSecure;
    s_api.get_prebuilt_data_dir_s = &RimeGetPrebuiltDataDirSecure;
    s_api.get_staging_dir_s = &RimeGetStagingDirSecure;
    s_api.get_sync_dir_s = &RimeGetSyncDirSecure;
    s_api.highlight_candidate = &RimeHighlightCandidate;
    s_api.highlight_candidate_on_current_page =
        &RimeHighlightCandidateOnCurrentPage;
    s_api.change_page = &RimeChangePage;
  }
  return &s_api;
}
