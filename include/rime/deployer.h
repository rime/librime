//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DEPLOYER_H_
#define RIME_DEPLOYER_H_

#include <future>
#include <mutex>
#include <queue>
#include <string>
#include <boost/any.hpp>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/messenger.h>

namespace rime {

class Deployer;

typedef boost::any TaskInitializer;

class DeploymentTask : public Class<DeploymentTask, TaskInitializer> {
 public:
  DeploymentTask() {}
  virtual ~DeploymentTask() {}

  virtual bool Run(Deployer* deployer) = 0;
};

class Deployer : public Messenger {
 public:
  // read-only access after library initialization {
  std::string shared_data_dir;
  std::string user_data_dir;
  std::string sync_dir;
  std::string user_id;
  std::string distribution_name;
  std::string distribution_code_name;
  std::string distribution_version;
  // }

  Deployer();
  ~Deployer();

  bool RunTask(const std::string& task_name,
               TaskInitializer arg = TaskInitializer());
  bool ScheduleTask(const std::string& task_name,
                    TaskInitializer arg = TaskInitializer());

  void ScheduleTask(const shared_ptr<DeploymentTask>& task);
  shared_ptr<DeploymentTask> NextTask();
  bool HasPendingTasks();

  bool Run();
  bool StartWork(bool maintenance_mode = false);
  bool StartMaintenance();
  bool IsWorking();
  bool IsMaintenanceMode();
  // the following two methods equally wait until all threads are joined
  void JoinWorkThread();
  void JoinMaintenanceThread();

  std::string user_data_sync_dir() const;

 private:
  std::queue<shared_ptr<DeploymentTask>> pending_tasks_;
  std::mutex mutex_;
  std::future<void> work_;
  bool maintenance_mode_;
};

}  // namespace rime

#endif  // RIME_DEPLOYER_H_
