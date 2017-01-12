//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-04-22 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DB_RECOVERY_TASK_H_
#define RIME_USER_DB_RECOVERY_TASK_H_

#include <rime/common.h>
#include <rime/deployer.h>

namespace rime {

class Db;

class UserDbRecoveryTask : public DeploymentTask {
 public:
  explicit UserDbRecoveryTask(an<Db> db);
  bool Run(Deployer* deployer);

 protected:
  void RestoreUserDataFromSnapshot(Deployer* deployer);

  an<Db> db_;
};

class UserDbRecoveryTaskComponent : public UserDbRecoveryTask::Component {
 public:
  UserDbRecoveryTask* Create(TaskInitializer arg);
};

}  // namespace rime

#endif  // RIME_USER_DB_RECOVERY_TASK_H_
