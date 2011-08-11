// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-09 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <rime.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/key_event.h>
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

RIME_API RimeComposition* RimeCreateComposition(RimeSessionId session_id) {
  rime::shared_ptr<rime::Session> session(g_service.GetSession(session_id));
  if (!session)
    return NULL;
  rime::Context *ctx = session->context();
  if (!ctx)
    return NULL;
  rime::Composition *comp = ctx->composition();
  if (!comp || comp->empty())
    return NULL;
  RimeComposition *result = new RimeComposition;
  if (!result)
    return NULL;
  std::memset(result, 0, sizeof(RimeComposition));
  result->ref_ = reinterpret_cast<void *>(comp);
  rime::Preedit preedit;
  comp->GetPreedit(&preedit);
  result->preedit = new char[preedit.text.length() + 1];
  if (result->preedit)
    std::strcpy(result->preedit, preedit.text.c_str());
  result->cursor_pos = preedit.cursor_pos;
  result->sel_start = preedit.sel_start;
  result->sel_end = preedit.sel_end;
  return result;
}

RIME_API bool RimeDestroyComposition(RimeComposition *composition) {
  if (!composition)
    return false;
  if (composition->preedit)
    delete[] composition->preedit;
  delete composition;
  return true;
}

RIME_API RimeMenu* RimeCreateMenu(RimeComposition *composition) {
  // TODO:
  return NULL;
}

RIME_API bool RimeDestroyMenu(RimeMenu *menu) {
  if (!menu)
    return false;
  // TODO:
  return false;
}
