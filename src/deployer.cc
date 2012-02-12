// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <rime/deployer.h>

namespace rime {

void Deployer::ScheduleTask(const shared_ptr<DeploymentTask>& task) {
  boost::lock_guard<boost::mutex> lock(mutex_);
  pending_tasks_.push(task);
}

shared_ptr<DeploymentTask> Deployer::NextTask() {
  boost::lock_guard<boost::mutex> lock(mutex_);
  shared_ptr<DeploymentTask> result;
  if (!pending_tasks_.empty()) {
    result = pending_tasks_.front();
    pending_tasks_.pop();
  }
  // there is still chance that a task is added by another thread
  // right after this call... careful.
  return result;
}

bool Deployer::Run() {
  EZLOGGERPRINT("running deployment tasks:");
  shared_ptr<DeploymentTask> task;
  int success = 0;
  int failure = 0;
  while ((task = NextTask())) {
    if (task->Run(this))
      ++success;
    else
      ++failure;
    boost::this_thread::interruption_point();
  }
  EZLOGGERPRINT("%d tasks ran: %d success, %d failure.",
                success + failure, success, failure);
  return failure == 0;
}

bool Deployer::StartMaintenance() {
  if (IsMaintenancing()) {
    EZLOGGERPRINT("Warning: a maintenance thread is already running.");
    return false;
  }
  if (pending_tasks_.empty()) {
    return false;
  }
  EZLOGGERPRINT("starting maintenance thread for %d tasks.",
                pending_tasks_.size());
  boost::thread t(boost::bind(&Deployer::Run, this));
  maintenance_thread_.swap(t);
  return maintenance_thread_.joinable();
}

bool Deployer::IsMaintenancing() {
  if (!maintenance_thread_.joinable())
    return false;
  return !maintenance_thread_.timed_join(boost::posix_time::milliseconds(0));
}

void Deployer::JoinMaintenanceThread() {
  maintenance_thread_.join();
}

}  // namespace rime
