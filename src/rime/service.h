//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-08 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SERVICE_H_
#define RIME_SERVICE_H_

#include <stdint.h>
#include <time.h>
#include <mutex>
#include <rime/common.h>
#include <rime/deployer.h>

namespace rime {

using SessionId = uintptr_t;

static const SessionId kInvalidSessionId = 0;

using NotificationHandler = function<void (SessionId session_id,
                                                const char* message_type,
                                                const char* message_value)>;

class Context;
class Engine;
class KeyEvent;
class Schema;

class Session {
 public:
  static const int kLifeSpan = 5 * 60;  // seconds

  Session();
  bool ProcessKey(const KeyEvent& key_event);
  void Activate();
  void ResetCommitText();
  bool CommitComposition();
  void ClearComposition();
  void ApplySchema(Schema* schema);

  Context* context() const;
  Schema* schema() const;
  time_t last_active_time() const { return last_active_time_; }
  const string& commit_text() const { return commit_text_; }

 private:
  void OnCommit(const string& commit_text);

  the<Engine> engine_;
  time_t last_active_time_ = 0;
  string commit_text_;
};

class ResourceResolver;
struct ResourceType;

class Service {
 public:
  ~Service();

  void StartService();
  void StopService();

  SessionId CreateSession();
  an<Session> GetSession(SessionId session_id);
  bool DestroySession(SessionId session_id);
  void CleanupStaleSessions();
  void CleanupAllSessions();

  void SetNotificationHandler(const NotificationHandler& handler);
  void ClearNotificationHandler();
  void Notify(SessionId session_id,
              const string& message_type,
              const string& message_value);

  ResourceResolver* CreateResourceResolver(const ResourceType& type);
  ResourceResolver* CreateUserSpecificResourceResolver(const ResourceType& type);

  Deployer& deployer() { return deployer_; }
  bool disabled() { return !started_ || deployer_.IsMaintenanceMode(); }

  static Service& instance();

 private:
  Service();

  using SessionMap = map<SessionId, an<Session>>;
  SessionMap sessions_;
  Deployer deployer_;
  NotificationHandler notification_handler_;
  std::mutex mutex_;
  bool started_ = false;
};

}  // namespace rime

#endif  // RIME_SERVICE_H_
