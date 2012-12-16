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
#include <map>
#include <boost/thread.hpp>
#include <rime/common.h>
#include <rime/deployer.h>

namespace rime {

typedef uintptr_t SessionId;

static const SessionId kInvalidSessionId = 0;

typedef boost::function<void (const char* message_type,
                              const char* message_value)> NotificationHandler;

class Context;
class Engine;
class KeyEvent;
class Schema;
class Switcher;

class Session {
 public:
  static const int kLifeSpan = 5 * 60;  // seconds

  Session();
  bool ProcessKeyEvent(const KeyEvent &key_event);
  void Activate();
  void ResetCommitText();
  bool CommitComposition();
  void ClearComposition();
  void ApplySchema(Schema* schema);

  Context* context() const;
  Schema* schema() const;
  const time_t last_active_time() const { return last_active_time_; }
  const std::string& commit_text() const { return commit_text_; }

 private:
  void OnCommit(const std::string &commit_text);

  scoped_ptr<Switcher> switcher_;
  scoped_ptr<Engine> engine_;
  time_t last_active_time_;
  std::string commit_text_;
};

class Service {
 public:
  ~Service();

  void StartService();
  void StopService();

  SessionId CreateSession();
  shared_ptr<Session> GetSession(SessionId session_id);
  bool DestroySession(SessionId session_id);
  void CleanupStaleSessions();
  void CleanupAllSessions();

  void SetNotificationHandler(const NotificationHandler& handler);
  void ClearNotificationHandler();
  void Notify(const std::string& message_type,
              const std::string& message_value);

  Deployer& deployer() { return deployer_; }
  bool disabled() { return !started_ || deployer_.IsMaintenancing(); }

  static Service& instance() {
    if (!instance_) instance_.reset(new Service);
    return *instance_;
  }
  
 private:
  Service();
  static scoped_ptr<Service> instance_;

  typedef std::map<SessionId, shared_ptr<Session> > SessionMap;
  SessionMap sessions_;
  Deployer deployer_;
  NotificationHandler notification_handler_;
  boost::mutex mutex_;
  bool started_;
};

}  // namespace rime

#endif  // RIME_SERVICE_H_
