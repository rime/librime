// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DEPLOYER_H_
#define RIME_DEPLOYER_H_

#include <string>
#include <queue>
#include <boost/thread.hpp>
#include <rime/common.h>

namespace rime {

int CompareVersionString(const std::string& x,
                         const std::string& y);

class Deployer;

class DeploymentTask {
 public:
  DeploymentTask() {}
  virtual ~DeploymentTask() {}
  
  virtual bool Run(Deployer* deployer) = 0;
};

class Deployer {
 public:
  // read-only access after library initialization {
  std::string shared_data_dir;
  std::string user_data_dir;
  std::string user_id;
  std::string distribution_name;
  std::string distribution_code_name;
  std::string distribution_version;
  // }

  Deployer() : shared_data_dir("."),
               user_data_dir("."),
               user_id("unknown") {}

  void ScheduleTask(const shared_ptr<DeploymentTask>& task);
  shared_ptr<DeploymentTask> NextTask();
  bool Run();
  bool StartMaintenance();
  bool IsMaintenancing();
  void JoinMaintenanceThread();

 private:
  std::queue<shared_ptr<DeploymentTask> > pending_tasks_;
  boost::mutex mutex_;
  boost::thread maintenance_thread_;
};

}  // namespace rime

#endif  // RIME_DEPLOYER_H_
