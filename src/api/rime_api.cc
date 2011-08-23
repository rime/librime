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
#include <rime.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/registry.h>
#include <rime/service.h>

static rime::Service g_service;

RIME_API void RimeInitialize() {
   rime::RegisterComponents();
}

RIME_API void RimeFinalize() {
   g_service.CleanupAllSessions();
   rime::Registry::instance().Clear();
}

// session management

RIME_API RimeSessionId RimeCreateSession() {
  return g_service.CreateSession();
}

RIME_API bool RimeDestroySession(RimeSessionId session_id) {
  return g_service.DestroySession(session_id);
}

RIME_API void RimeCleanupStaleSessions() {
  g_service.CleanupStaleSessions();
}

RIME_API void RimeCleanupAllSessions() {
  g_service.CleanupAllSessions();
}

// using sessions

RIME_API bool RimeProcessKey(RimeSessionId session_id, int keycode, int mask) {
  rime::shared_ptr<rime::Session> session(g_service.GetSession(session_id));
  if (!session)
    return false;
  return session->ProcessKeyEvent(rime::KeyEvent(keycode, mask));
}

RIME_API bool RimeGetContext(RimeSessionId session_id, RimeContext *context) {
  if (!context)
    return false;
  std::memset(context, 0, sizeof(RimeContext));
  rime::shared_ptr<rime::Session> session(g_service.GetSession(session_id));
  if (!session)
    return false;
  rime::Context *ctx = session->context();
  if (!ctx)
    return false;
  rime::Composition *comp = ctx->composition();
  if (comp && !comp->empty()) {
    rime::Preedit preedit;
    comp->GetPreedit(&preedit);
    std::strncpy(context->composition.preedit, preedit.text.c_str(),
                 kRimeTextMaxLength);
    context->composition.cursor_pos = preedit.cursor_pos;
    context->composition.sel_start = preedit.sel_start;
    context->composition.sel_end = preedit.sel_end;
    if (comp->back().menu) {
      // TODO: read schema settings or defaults
      int page_size = 5;
      int selected_index = comp->back().selected_index;
      int page_no = selected_index / page_size;
      rime::scoped_ptr<rime::Page> page(
          comp->back().menu->CreatePage(page_size, page_no));
      context->menu.page_size = page_size;
      context->menu.page_no = page_no;
      context->menu.is_last_page = page->is_last_page;
      context->menu.highlighted_candidate_index = selected_index % page_size;
      int i = 0;
      BOOST_FOREACH(const rime::shared_ptr<rime::Candidate> &cand,
                    page->candidates) {
        std::strncpy(context->menu.candidates[i], cand->text().c_str(),
                     kRimeTextMaxLength);
        if (++i >= kRimeMaxNumCandidates) break;
      }
      context->menu.num_candidates = i;
    }
  }
  return true;
}

RIME_API bool RimeGetCommit(RimeSessionId session_id, RimeCommit* commit) {
  if (!commit)
    return false;
  std::memset(commit, 0, sizeof(RimeCommit));
  rime::shared_ptr<rime::Session> session(g_service.GetSession(session_id));
  if (!session)
    return false;
  // TODO:
  return false;
}

RIME_API bool RimeGetStatus(RimeSessionId session_id, RimeStatus* status) {
  if (!status)
    return false;
  std::memset(status, 0, sizeof(RimeStatus));
  rime::shared_ptr<rime::Session> session(g_service.GetSession(session_id));
  if (!session)
    return false;
  // TODO:
  return false;
}
