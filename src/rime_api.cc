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
#include <rime/expl/signature.h>
#include <rime_api.h>


RIME_API void RimeInitialize(RimeTraits *traits) {
  RimeDeployerInitialize(traits);
  rime::RegisterComponents();
  rime::Service::instance().StartService();
}

RIME_API void RimeFinalize() {
  RimeJoinMaintenanceThread();
  rime::Service::instance().StopService();
  rime::Registry::instance().Clear();
}

RIME_API Bool RimeStartMaintenance(Bool full_check) {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::InstallationUpdate installation;
  installation.Run(&deployer);
  if (!full_check) {
    rime::ConfigFileUpdate default_config_update("default.yaml",
                                                 "config_version");
    bool updated = default_config_update.Run(&deployer);
    if (!updated) {
      return False;
    }
    else {
      EZLOGGERPRINT("changes detected; starting maintenance.");
    }
  }
  deployer.ScheduleTask(boost::make_shared<rime::WorkspaceUpdate>());
  deployer.ScheduleTask(boost::make_shared<rime::UserDictUpgration>());
  deployer.StartMaintenance();
  return True;
}

RIME_API Bool RimeStartMaintenanceOnWorkspaceChange() {
  return RimeStartMaintenance(False);
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
  if (!traits) return;
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

RIME_API Bool RimePrebuildAllSchemas() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::PrebuildAllSchemas prebuild;
  return Bool(prebuild.Run(&deployer));
}

RIME_API Bool RimeDeployWorkspace() {
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::InstallationUpdate installation;
  rime::WorkspaceUpdate update;
  rime::UserDictUpgration upgration;
  return Bool(installation.Run(&deployer) &&
              update.Run(&deployer) &&
              upgration.Run(&deployer));
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
  return Bool(session->ProcessKeyEvent(rime::KeyEvent(keycode, mask)));
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
  std::memset((char*)context + sizeof(context->data_size), 0, context->data_size);
  boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return False;
  rime::Context *ctx = session->context();
  if (!ctx)
    return False;
  if (ctx->IsComposing()) {
    rime::Preedit preedit;
    ctx->GetPreedit(&preedit);
    context->composition.length = preedit.text.length();
    context->composition.preedit = new char[preedit.text.length() + 1];
    std::strcpy(context->composition.preedit, preedit.text.c_str());
    context->composition.cursor_pos = preedit.caret_pos;
    context->composition.sel_start = preedit.sel_start;
    context->composition.sel_end = preedit.sel_end;
    if (RIME_STRUCT_HAS_MEMBER(*context, context->commit_text_preview)) {
      const std::string commit_text(ctx->GetCommitText());
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
      BOOST_FOREACH(const boost::shared_ptr<rime::Candidate> &cand, page->candidates) {
        RimeCandidate* dest = &context->menu.candidates[i];
        dest->text = new char[cand->text().length() + 1];
        std::strcpy(dest->text, cand->text().c_str());
        const std::string comment(cand->comment());
        if (!comment.empty()) {
          dest->comment = new char[comment.length() + 1];
          std::strcpy(dest->comment, comment.c_str());
        }
        if (++i >= RIME_MAX_NUM_CANDIDATES) break;
      }
      context->menu.num_candidates = i;
      if (schema) {
        const std::string& select_keys(schema->alternative_select_keys());
        if (!select_keys.empty()) {
          std::strncpy(context->menu.select_keys, select_keys.c_str(),
                       RIME_MAX_NUM_CANDIDATES);
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
  if (RIME_STRUCT_HAS_MEMBER(*context, context->commit_text_preview)) {
    delete[] context->commit_text_preview;
  }
  std::memset((char*)context + sizeof(context->data_size), 0, context->data_size);
  return True;
}

RIME_API Bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit) {
  if (!commit)
    return False;
  std::memset(commit, 0, sizeof(RimeCommit));
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
  std::memset(commit, 0, sizeof(RimeCommit));
  return True;
}

RIME_API Bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status) {
  if (!status || status->data_size <= 0)
    return False;
  std::memset((char*)status + sizeof(status->data_size), 0, status->data_size);
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
  return True;
}

RIME_API Bool RimeFreeStatus(RimeStatus* status) {
  if (!status || status->data_size <= 0)
    return False;
  delete[] status->schema_id;
  delete[] status->schema_name;
  std::memset((char*)status + sizeof(status->data_size), 0, status->data_size);
  return True;
}

RIME_API Bool RimeConfigOpen(const char *config_id, RimeConfig* config) {
  if (!config || !config) return False;
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
  std::string str_value;
  if (c->GetString(key, &str_value)) {
    std::strncpy(value, str_value.c_str(), buffer_size);
    return True;
  }
  return False;
}

RIME_API Bool RimeConfigUpdateSignature(RimeConfig *config, const char* signer) {
  if (!config || !signer) return False;
  rime::Config *c = reinterpret_cast<rime::Config*>(config->ptr);
  rime::Deployer &deployer(rime::Service::instance().deployer());
  rime::Signature sig(signer);
  return Bool(sig.Sign(c, &deployer));
}

RIME_API Bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence) {
    EZLOGGERVAR(key_sequence);
    boost::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
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
