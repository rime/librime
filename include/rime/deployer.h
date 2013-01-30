//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DEPLOYER_H_
#define RIME_DEPLOYER_H_

#include <queue>
#include <string>
#include <boost/thread.hpp>
#include <rime/common.h>
#include <rime/messenger.h>

namespace rime {

class Deployer;

class DeploymentTask {
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

  Deployer() : shared_data_dir("."),
               user_data_dir("."),
               sync_dir("sync"),
               user_id("unknown") {}

  void ScheduleTask(const shared_ptr<DeploymentTask>& task);
  shared_ptr<DeploymentTask> NextTask();
  bool HasPendingTasks();
  
  bool Run();
  bool StartMaintenance();
  bool IsMaintenancing();
  void JoinMaintenanceThread();

  const std::string user_data_sync_dir() const;

 private:
  std::queue<shared_ptr<DeploymentTask> > pending_tasks_;
  boost::mutex mutex_;
  boost::thread maintenance_thread_;
};

}  // namespace rime

#endif  // RIME_DEPLOYER_H_
