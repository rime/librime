// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-08-08 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/switcher.h>

namespace rime {

scoped_ptr<Service> Service::instance_;

Session::Session() : last_active_time_(0) {
  switcher_.reset(new Switcher);
  engine_.reset(new Engine(switcher_->CreateSchema()));
  engine_->sink().connect(boost::bind(&Session::OnCommit, this, _1));
}

bool Session::ProcessKeyEvent(const KeyEvent &key_event) {
  return switcher_->ProcessKeyEvent(key_event) ||
      engine_->ProcessKeyEvent(key_event);
}

void Session::Activate() {
  last_active_time_ = time(NULL);
}

void Session::ResetCommitText() {
  commit_text_.clear();
}

void Session::OnCommit(const std::string &commit_text) {
  commit_text_ += commit_text;
}

Context* Session::context() const {
  if (switcher_->active()) {
    return switcher_->context();
  }
  return engine_ ? engine_->context() : NULL;
}

Schema* Session::schema() const {
  return engine_ ? engine_->schema() : NULL;
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
    session->Activate();
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
  if (it == sessions_.end()) {
    return shared_ptr<Session>();
  }
  else {
    it->second->Activate();
    return it->second;
  }
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
