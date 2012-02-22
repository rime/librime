// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-09 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/deployer.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/registry.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/expl/deployment_tasks.h>
#include <rime_api.h>


RIME_API void RimeInitialize(RimeTraits *traits) {
  RimeDeployerInitialize(traits);
  rime::RegisterComponents();
}

RIME_API void RimeFinalize() {
  RimeJoinMaintenanceThread();
  rime::Service::instance().CleanupAllSessions();
  rime::Registry::instance().Clear();
}

RIME_API Bool RimeStartMaintenanceOnWorkspaceChange() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::InstallationUpdate installation;
  installation.Run(&deployer);
  rime::ConfigFileUpdate default_config_update("default.yaml",
                                               "config_version");
  bool updated = default_config_update.Run(&deployer);
  if (updated) {
    EZLOGGERPRINT("changes detected; starting maintenance.");
    rime::shared_ptr<rime::DeploymentTask> task(new rime::WorkspaceUpdate);
    deployer.ScheduleTask(task);
    deployer.StartMaintenance();
    return True;
  }
  return False;
}

RIME_API Bool RimeIsMaintenancing() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  return Bool(deployer.IsMaintenancing());
}

RIME_API void RimeJoinMaintenanceThread() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  deployer.JoinMaintenanceThread();
}

// deployment

RIME_API void RimeDeployerInitialize(RimeTraits *traits) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  deployer.shared_data_dir = traits->shared_data_dir;
  deployer.user_data_dir = traits->user_data_dir;
  if (traits->distribution_name)
    deployer.distribution_name = traits->distribution_name;
  if (traits->distribution_code_name)
    deployer.distribution_code_name = traits->distribution_code_name;
  if (traits->distribution_version)
    deployer.distribution_version = traits->distribution_version;
}

RIME_API Bool RimeDeployWorkspace() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::InstallationUpdate installation;
  rime::WorkspaceUpdate update;
  return Bool(installation.Run(&deployer) && update.Run(&deployer));
}

RIME_API Bool RimeDeploySchema(const char *schema_file) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::SchemaUpdate update(schema_file);
  return Bool(update.Run(&deployer));
}

RIME_API Bool RimeDeployConfigFile(const char *file_name,
                                   const char *version_key) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::ConfigFileUpdate update(file_name, version_key);
  return Bool(update.Run(&deployer));
}

// session management

RIME_API RimeSessionId RimeCreateSession() {
  return rime::Service::instance().CreateSession();
}

RIME_API Bool RimeFindSession(RimeSessionId session_id) {
  return Bool(rime::Service::instance().GetSession(session_id) != 0);
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

// using sessions

RIME_API Bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask) {
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  return Bool(session->ProcessKeyEvent(rime::KeyEvent(keycode, mask)));
}

RIME_API Bool RimeGetContext(RimeSessionId session_id, RimeContext *context) {
  if (!context)
    return False;
  std::memset(context, 0, sizeof(RimeContext));
  rime::shared_ptr<rime::Session> session(
      rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Context *ctx = session->context();
  if (!ctx)
    return False;
  if (ctx->IsComposing()) {
    rime::Preedit preedit;
    ctx->GetPreedit(&preedit);
    context->composition.length = preedit.text.length();
    std::strncpy(context->composition.preedit,
                 preedit.text.c_str(), RIME_TEXT_MAX_LENGTH);
    context->composition.cursor_pos = preedit.caret_pos;
    context->composition.sel_start = preedit.sel_start;
    context->composition.sel_end = preedit.sel_end;
  }
  if (ctx->HasMenu()) {
    rime::Segment &seg(ctx->composition()->back());
    int page_size = 5;
    rime::Schema *schema = session->schema();
    if (schema)
      page_size = schema->page_size();
    int selected_index = seg.selected_index;
    int page_no = selected_index / page_size;
    rime::scoped_ptr<rime::Page> page(
        seg.menu->CreatePage(page_size, page_no));
    if (page) {
      context->menu.page_size = page_size;
      context->menu.page_no = page_no;
      context->menu.is_last_page = Bool(page->is_last_page);
      context->menu.highlighted_candidate_index = selected_index % page_size;
      int i = 0;
      BOOST_FOREACH(const rime::shared_ptr<rime::Candidate> &cand,
                    page->candidates) {
        std::string candidate(cand->text());
        if (!cand->comment().empty()) {
          candidate += "  " + cand->comment();
        }
        char *dest = context->menu.candidates[i];
        std::strncpy(dest, candidate.c_str(), RIME_TEXT_MAX_LENGTH);
        if (++i >= RIME_MAX_NUM_CANDIDATES) break;
      }
      context->menu.num_candidates = i;
    }
  }
  return True;
}

RIME_API Bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit) {
  if (!commit)
    return False;
  std::memset(commit, 0, sizeof(RimeCommit));
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  if (!session->commit_text().empty()) {
    std::strncpy(commit->text, session->commit_text().c_str(),
                 RIME_TEXT_MAX_LENGTH);
    session->ResetCommitText();
    return True;
  }
  return False;
}

RIME_API Bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status) {
  if (!status)
    return False;
  std::memset(status, 0, sizeof(RimeStatus));
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Schema *schema = session->schema();
  rime::Context *ctx = session->context();
  if (!schema || !ctx)
    return False;
  std::strncpy(status->schema_id, schema->schema_id().c_str(),
               RIME_SCHEMA_MAX_LENGTH);
  std::strncpy(status->schema_name, schema->schema_name().c_str(),
               RIME_SCHEMA_MAX_LENGTH);
  // TODO:
  status->is_disabled = False;
  status->is_composing = Bool(ctx->IsComposing());
  status->is_ascii_mode = Bool(ctx->get_option("ascii_mode"));
  status->is_full_shape = Bool(ctx->get_option("full_shape"));
  status->is_simplified = Bool(ctx->get_option("simplification"));
  return True;
}

RIME_API Bool RimeConfigOpen(const char *config_id, RimeConfig* config) {
  if (!config || !config) return False;
  rime::Config *c = rime::Config::Require("config")->Create(config_id);
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
  std::string str_value;
  if (c->GetString(key, &str_value)) {
    std::strncpy(value, str_value.c_str(), buffer_size);
    return True;
  }
  return False;
}

RIME_API Bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence) {
    EZLOGGERVAR(key_sequence);
    rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
    if (!session)
      return False;
    rime::KeySequence keys;
    if (!keys.Parse(key_sequence)) {
      EZLOGGERPRINT("error parsing input: '%s'", key_sequence);
      return False;
    }
    BOOST_FOREACH(const rime::KeyEvent &ke, keys) {
      session->ProcessKeyEvent(ke);
    }
    return True;
}
