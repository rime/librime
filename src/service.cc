// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-08 GONG Chen <chen.sst@gmail.com>
//
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/service.h>

namespace rime {

Session::Session() : engine_(new Engine),
                     last_active_time_(time(NULL)) {
  engine_->set_schema(new Schema);
}

bool Session::ProcessKeyEvent(const KeyEvent &key_event) {
  last_active_time_ = time(NULL);
  return engine_->ProcessKeyEvent(key_event);
}

Context* Session::context() const {
  return engine_ ? engine_->context() : NULL;
}

Service::Service() {
}

Service::~Service() {
  CleanupAllSessions();
}

SessionId Service::CreateSession() {
  SessionId id = kInvalidSessionId;
  try {
    shared_ptr<Session> session(new Session);
    id = reinterpret_cast<uintptr_t>(session.get());
    sessions_[id] = session;
  }
  catch (...) {
    EZLOGGERPRINT("Error creating session.");
  }
  return id;
}

shared_ptr<Session> Service::GetSession(SessionId session_id) {
  SessionMap::iterator it = sessions_.find(session_id);
  if (it == sessions_.end())
    return shared_ptr<Session>();
  else
    return it->second;
}

bool Service::DestroySession(SessionId session_id) {
  SessionMap::iterator it = sessions_.find(session_id);
  if (it == sessions_.end())
    return false;
  sessions_.erase(it);
  return true;
}

void Service::CleanupStaleSessions() {
  const time_t now = time(NULL);
  int count = 0;
  for (SessionMap::iterator it = sessions_.begin();
       it != sessions_.end(); ) {
    if (it->second &&
        it->second->last_active_time() < now - Session::kLifeSpan) {
      sessions_.erase(it++);
      ++count;
    }
    else
      ++it;
  }
  if (count > 0) {
    EZLOGGERPRINT("Recycled %d stale sessions.");
  }
}

void Service::CleanupAllSessions() {
  sessions_.clear();
}

}  // namespace rime
