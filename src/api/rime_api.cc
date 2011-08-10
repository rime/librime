// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-09 GONG Chen <chen.sst@gmail.com>
//
#include <rime.h>
#include <rime/common.h>
#include <rime/component.h>
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

