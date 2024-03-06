//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DEPLOYER_H_
#define RIME_DEPLOYER_H_

#include <future>
#include <mutex>
#include <queue>
#include <any>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/messenger.h>

namespace rime {

class Deployer;

using TaskInitializer = std::any;

class DeploymentTask : public Class<DeploymentTask, TaskInitializer> {
 public:
  DeploymentTask() = default;
  virtual ~DeploymentTask() = default;

  virtual bool Run(Deployer* deployer) = 0;
};

class Deployer : public Messenger {
 public:
  // read-only access after library initialization {
  path shared_data_dir;
  path user_data_dir;
  path prebuilt_data_dir;
  path staging_dir;
  path sync_dir;
  string user_id;
  string distribution_name;
  string distribution_code_name;
  string distribution_version;
  string app_name;
  // }

  RIME_API Deployer();
  RIME_API ~Deployer();

  bool RunTask(const string& task_name,
               TaskInitializer arg = TaskInitializer());
  bool ScheduleTask(const string& task_name,
                    TaskInitializer arg = TaskInitializer());
  void ScheduleTask(an<DeploymentTask> task);
  an<DeploymentTask> NextTask();
  bool HasPendingTasks();

  bool Run();
  bool StartWork(bool maintenance_mode = false);
  bool StartMaintenance();
  bool IsWorking();
  bool IsMaintenanceMode();
  // the following two methods equally wait until all threads are joined
  void JoinWorkThread();
  void JoinMaintenanceThread();

  path user_data_sync_dir() const;

 private:
  std::queue<of<DeploymentTask>> pending_tasks_;
  std::mutex mutex_;
  std::future<void> work_;
  bool maintenance_mode_ = false;
};

}  // namespace rime

#endif  // RIME_DEPLOYER_H_
