//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-08 GONG Chen <chen.sst@gmail.com>
//
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/resource.h>
#include <rime/schema.h>
#include <rime/service.h>

using namespace std::placeholders;

namespace rime {

Session::Session() {
  engine_.reset(Engine::Create());
  engine_->sink().connect(std::bind(&Session::OnCommit, this, _1));
  SessionId session_id = reinterpret_cast<SessionId>(this);
  engine_->message_sink().connect(
      std::bind(&Service::Notify, &Service::instance(), session_id, _1, _2));
}

bool Session::ProcessKey(const KeyEvent& key_event) {
  return engine_->ProcessKey(key_event);
}

void Session::Activate() {
  last_active_time_ = time(NULL);
}

void Session::ResetCommitText() {
  commit_text_.clear();
}

bool Session::CommitComposition() {
  if (!engine_)
    return false;
  engine_->context()->Commit();
  return !commit_text_.empty();
}

void Session::ClearComposition() {
  if (!engine_)
    return;
  engine_->context()->Clear();
}

void Session::ApplySchema(Schema* schema) {
  engine_->ApplySchema(schema);
}

void Session::OnCommit(const string& commit_text) {
  commit_text_ += commit_text;
}

Context* Session::context() const {
  return engine_ ? engine_->active_context() : NULL;
}

Schema* Session::schema() const {
  return engine_ ? engine_->schema() : NULL;
}

Service::Service() {
  deployer_.message_sink().connect(
      std::bind(&Service::Notify, this, 0, _1, _2));
}

Service::~Service() {
  StopService();
}

void Service::StartService() {
  started_ = true;
}

void Service::StopService() {
  started_ = false;
  CleanupAllSessions();
}

SessionId Service::CreateSession() {
  SessionId id = kInvalidSessionId;
  if (disabled()) return id;
  try {
    auto session = New<Session>();
    session->Activate();
    id = reinterpret_cast<uintptr_t>(session.get());
    sessions_[id] = session;
  }
  catch (const std::exception& ex) {
    LOG(ERROR) << "Error creating session: " << ex.what();
  }
  catch (const string& ex) {
    LOG(ERROR) << "Error creating session: " << ex;
  }
  catch (const char* ex) {
    LOG(ERROR) << "Error creating session: " << ex;
  }
  catch (int ex) {
    LOG(ERROR) << "Error creating session: " << ex;
  }
  catch (...) {
    LOG(ERROR) << "Error creating session.";
  }
  return id;
}

an<Session> Service::GetSession(SessionId session_id) {
  if (disabled())
    return nullptr;
  SessionMap::iterator it = sessions_.find(session_id);
  if (it != sessions_.end()) {
    auto& session = it->second;
    session->Activate();
    return session;
  }
  return nullptr;
}

bool Service::DestroySession(SessionId session_id) {
  auto it = sessions_.find(session_id);
  if (it == sessions_.end())
    return false;
  sessions_.erase(it);
  return true;
}

void Service::CleanupStaleSessions() {
  time_t now = time(NULL);
  int count = 0;
  for (auto it = sessions_.begin(); it != sessions_.end(); ) {
    if (it->second &&
        it->second->last_active_time() < now - Session::kLifeSpan) {
      sessions_.erase(it++);
      ++count;
    }
    else {
      ++it;
    }
  }
  if (count > 0) {
    LOG(INFO) << "Recycled " << count << " stale sessions.";
  }
}

void Service::CleanupAllSessions() {
  sessions_.clear();
}

void Service::SetNotificationHandler(const NotificationHandler& handler) {
  notification_handler_ = handler;
}

void Service::ClearNotificationHandler() {
  notification_handler_ = nullptr;
}

void Service::Notify(SessionId session_id,
                     const string& message_type,
                     const string& message_value) {
  if (notification_handler_) {
    std::lock_guard<std::mutex> lock(mutex_);
    notification_handler_(session_id,
                          message_type.c_str(),
                          message_value.c_str());
  }
}

ResourceResolver* Service::CreateResourceResolver(const ResourceType& type) {
  the<FallbackResourceResolver> resolver(new FallbackResourceResolver(type));
  resolver->set_root_path(deployer().user_data_dir);
  resolver->set_fallback_root_path(deployer().shared_data_dir);
  return resolver.release();
}

ResourceResolver* Service::CreateUserSpecificResourceResolver(
    const ResourceType& type) {
  the<ResourceResolver> resolver(new ResourceResolver(type));
  resolver->set_root_path(deployer().user_data_dir);
  return resolver.release();
}

Service& Service::instance() {
  static the<Service> s_instance;
  if (!s_instance) {
    s_instance.reset(new Service);
  }
  return *s_instance;
}

}  // namespace rime
