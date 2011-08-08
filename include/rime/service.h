// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-08 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SERVICE_H_
#define RIME_SERVICE_H_

#include <stdint.h>
#include <rime/common.h>

namespace rime {

typedef uintptr_t SessionId;

static const SessionId kInvalidSessionId = 0;

class Engine;
class KeyEvent;

class Session {
 public:
  Session();
  bool ProcessKeyEvent(const KeyEvent &key_event);

 private:
  scoped_ptr<Engine> engine_;
};

class Service {
 public:
  Service();
  ~Service();
  
  SessionId CreateSession();
  shared_ptr<Session> GetSession(SessionId session_id);
  bool DestroySession(SessionId session_id);
  void CleanupStaleSessions();
  void CleanupAllSessions();
  
 private:
  typedef std::map<SessionId, shared_ptr<Session> > SessionMap;
  SessionMap sessions_;
};

}  // namespace rime

#endif  // RIME_SERVICE_H_
