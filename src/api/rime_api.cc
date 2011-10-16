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
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/registry.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime_api.h>

RIME_API void RimeInitialize(RimeTraits *traits) {
  if (traits) {
    rime::ConfigComponent::set_shared_data_dir(traits->shared_data_dir);
    rime::ConfigComponent::set_user_data_dir(traits->user_data_dir);
  }
  rime::RegisterComponents();
}

RIME_API void RimeInitialize() {
  rime::RegisterComponents();
}

RIME_API void RimeFinalize() {
  rime::Service::instance().CleanupAllSessions();
  rime::Registry::instance().Clear();
}

// session management

RIME_API RimeSessionId RimeCreateSession() {
  return rime::Service::instance().CreateSession();
}

RIME_API bool RimeFindSession(RimeSessionId session_id) {
  return bool(rime::Service::instance().GetSession(session_id));
}

RIME_API bool RimeDestroySession(RimeSessionId session_id) {
  return rime::Service::instance().DestroySession(session_id);
}

RIME_API void RimeCleanupStaleSessions() {
  rime::Service::instance().CleanupStaleSessions();
}

RIME_API void RimeCleanupAllSessions() {
  rime::Service::instance().CleanupAllSessions();
}

// using sessions

RIME_API bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask) {
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return false;
  return session->ProcessKeyEvent(rime::KeyEvent(keycode, mask));
}

RIME_API bool RimeGetContext(RimeSessionId session_id, RimeContext *context) {
  if (!context)
    return false;
  std::memset(context, 0, sizeof(RimeContext));
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return false;
  rime::Context *ctx = session->context();
  if (!ctx)
    return false;
  rime::Composition *comp = ctx->composition();
  if (comp && !comp->empty()) {
    context->composition.is_composing = true;
    rime::Preedit preedit;
    comp->GetPreedit(&preedit);
    std::strncpy(context->composition.preedit, preedit.text.c_str(),
                 kRimeTextMaxLength);
    context->composition.cursor_pos = preedit.cursor_pos;
    context->composition.sel_start = preedit.sel_start;
    context->composition.sel_end = preedit.sel_end;
    if (comp->back().menu) {
      int page_size = 5;
      rime::Schema *schema = session->schema();
      if (schema)
        page_size = schema->page_size();
      int selected_index = comp->back().selected_index;
      int page_no = selected_index / page_size;
      rime::scoped_ptr<rime::Page> page(
          comp->back().menu->CreatePage(page_size, page_no));
      if (page) {
        context->menu.page_size = page_size;
        context->menu.page_no = page_no;
        context->menu.is_last_page = page->is_last_page;
        context->menu.highlighted_candidate_index = selected_index % page_size;
        int i = 0;
        BOOST_FOREACH(const rime::shared_ptr<rime::Candidate> &cand,
                      page->candidates) {
          // TODO:
          std::string candidate_text(cand->text());
          if (!cand->prompt().empty()) {
            candidate_text += "  " + cand->prompt();
          }
          std::strncpy(context->menu.candidates[i], candidate_text.c_str(),
                       kRimeTextMaxLength);
          if (++i >= kRimeMaxNumCandidates) break;
        }
        context->menu.num_candidates = i;
      }
    }
  }
  return true;
}

RIME_API bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit) {
  if (!commit)
    return false;
  std::memset(commit, 0, sizeof(RimeCommit));
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return false;
  if (!session->commit_text().empty()) {
    std::strncpy(commit->text, session->commit_text().c_str(),
                 kRimeTextMaxLength);
    session->ResetCommitText();
    return true;
  }
  return false;
}

RIME_API bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status) {
  if (!status)
    return false;
  std::memset(status, 0, sizeof(RimeStatus));
  rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
  if (!session)
    return false;
  rime::Schema *schema = session->schema();
  if (!schema)
    return false;
  std::strncpy(status->schema_id, schema->schema_id().c_str(),
               kRimeSchemaMaxLength);
  std::strncpy(status->schema_name, schema->schema_name().c_str(),
               kRimeSchemaMaxLength);
  // TODO:
  status->is_disabled = false;
  status->is_ascii_mode = false;
  status->is_simplified = false;
  return true;
}

RIME_API bool RimeSimulateKeySequence(RimeSessionId session_id, const char *key_sequence) {
    EZLOGGERVAR(key_sequence);
    rime::shared_ptr<rime::Session> session(rime::Service::instance().GetSession(session_id));
    if (!session)
      return false;
    rime::KeySequence keys;
    if (!keys.Parse(key_sequence)) {
      EZLOGGERPRINT("error parsing input: '%s'", key_sequence);
      return false;
    }
    BOOST_FOREACH(const rime::KeyEvent &ke, keys) {
      session->ProcessKeyEvent(ke);
    }
    return true;
}
