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
#include <time.h>
#include <rime/common.h>

namespace rime {

typedef uintptr_t SessionId;

static const SessionId kInvalidSessionId = 0;

class Context;
class Engine;
class KeyEvent;
class Schema;

class Session {
 public:
  static const int kLifeSpan = 5 * 60;  // seconds
  
  Session();
  bool ProcessKeyEvent(const KeyEvent &key_event);
  void Activate();
  void ResetCommitText();

  Context* context() const;
  Schema* schema() const;
  const time_t last_active_time() const { return last_active_time_; }
  const std::string& commit_text() const { return commit_text_; }
  
 private:
  void OnCommit(const std::string &commit_text);

  scoped_ptr<Engine> engine_;
  time_t last_active_time_;
  std::string commit_text_;
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
