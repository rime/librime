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
  using SchemaDeploymentNotifier =
      signal<void(Deployer* deployer, const string& schema_id)>;

  // Synchronous notifications on the deployment caller's thread. Before is
  // emitted once the schema ID/source is known, before updating its config.
  // After pairs with Before on normal returns, including deployment failures;
  // it does not guarantee valid artifacts. Workspace After notifications wait
  // until all dictionary compile units have finished, and follow Before order.
  // Slots must handle their own errors/exceptions and must not reenter
  // deployment. Disconnect plugin slots before unloading the plugin.
  SchemaDeploymentNotifier& before_schema_deploy() {
    return before_schema_deploy_;
  }
  SchemaDeploymentNotifier& after_schema_deploy() {
    return after_schema_deploy_;
  }

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
  bool backup_config_files;
  // }

  RIME_DLL Deployer();
  RIME_DLL ~Deployer();

  RIME_DLL bool RunTask(const string& task_name,
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
  SchemaDeploymentNotifier before_schema_deploy_;
  SchemaDeploymentNotifier after_schema_deploy_;
  std::queue<of<DeploymentTask>> pending_tasks_;
  std::mutex mutex_;
  std::future<void> work_;
  bool maintenance_mode_ = false;
};

}  // namespace rime

#endif  // RIME_DEPLOYER_H_
