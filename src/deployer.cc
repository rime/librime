//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <rime/deployer.h>

namespace rime {

const std::string Deployer::user_data_sync_dir() const {
  boost::filesystem::path p(sync_dir);
  p /= user_id;
  return p.string();
}

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

bool Deployer::HasPendingTasks() {
    boost::lock_guard<boost::mutex> lock(mutex_);
    return !pending_tasks_.empty();
}

bool Deployer::Run() {
  LOG(INFO) << "running deployment tasks:";
  message_sink_("deploy", "start");
  shared_ptr<DeploymentTask> task;
  int success = 0;
  int failure = 0;
  do {
    while ((task = NextTask())) {
      if (task->Run(this))
        ++success;
      else
        ++failure;
      boost::this_thread::interruption_point();
    }
    LOG(INFO) << success + failure << " tasks ran: "
              << success << " success, " << failure << " failure.";
    message_sink_("deploy", !failure ? "success" : "failure");
    // new tasks could have been enqueued while we were sending the message.
    // before quitting, double check if there is nothing left to do.
  }
  while (HasPendingTasks());
  return !failure;
}

bool Deployer::StartMaintenance() {
  if (IsMaintenancing()) {
    LOG(WARNING) << "a maintenance thread is already running.";
    return false;
  }
  if (pending_tasks_.empty()) {
    return false;
  }
  LOG(INFO) << "starting maintenance thread for "
            << pending_tasks_.size() << " tasks.";
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
  if (!maintenance_thread_.joinable())
    return;
  maintenance_thread_.join();
}

}  // namespace rime
