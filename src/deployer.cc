//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#include <chrono>
#include <utility>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <rime/deployer.h>

namespace rime {

Deployer::Deployer() : shared_data_dir("."),
                       user_data_dir("."),
                       sync_dir("sync"),
                       user_id("unknown") {
}

Deployer::~Deployer() {
  JoinWorkThread();
}

string Deployer::user_data_sync_dir() const {
  boost::filesystem::path p(sync_dir);
  p /= user_id;
  return p.string();
}

bool Deployer::RunTask(const string& task_name, TaskInitializer arg) {
  auto c = DeploymentTask::Require(task_name);
  if (!c) {
    LOG(ERROR) << "unknown deployment task: " << task_name;
    return false;
  }
  the<DeploymentTask> t(c->Create(arg));
  if (!t) {
    LOG(ERROR) << "error creating deployment task: " << task_name;
    return false;
  }
  return t->Run(this);
}

bool Deployer::ScheduleTask(const string& task_name, TaskInitializer arg) {
  auto c = DeploymentTask::Require(task_name);
  if (!c) {
    LOG(ERROR) << "unknown deployment task: " << task_name;
    return false;
  }
  an<DeploymentTask> t(c->Create(arg));
  if (!t) {
    LOG(ERROR) << "error creating deployment task: " << task_name;
    return false;
  }
  ScheduleTask(t);
  return true;
}

void Deployer::ScheduleTask(an<DeploymentTask> task) {
  std::lock_guard<std::mutex> lock(mutex_);
  pending_tasks_.push(task);
}

an<DeploymentTask> Deployer::NextTask() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!pending_tasks_.empty()) {
    auto result = pending_tasks_.front();
    pending_tasks_.pop();
    return result;
  }
  // there is still chance that a task is added by another thread
  // right after this call... careful.
  return nullptr;
}

bool Deployer::HasPendingTasks() {
  std::lock_guard<std::mutex> lock(mutex_);
  return !pending_tasks_.empty();
}

bool Deployer::Run() {
  LOG(INFO) << "running deployment tasks:";
  message_sink_("deploy", "start");
  int success = 0;
  int failure = 0;
  do {
    while (auto task = NextTask()) {
      if (task->Run(this))
        ++success;
      else
        ++failure;
      //boost::this_thread::interruption_point();
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

bool Deployer::StartWork(bool maintenance_mode) {
  if (IsWorking()) {
    LOG(WARNING) << "a work thread is already running.";
    return false;
  }
  maintenance_mode_ = maintenance_mode;
  if (pending_tasks_.empty()) {
    return false;
  }
  LOG(INFO) << "starting work thread for "
            << pending_tasks_.size() << " tasks.";
  work_ = std::async(std::launch::async, [this] { Run(); });
  return work_.valid();
}

bool Deployer::StartMaintenance() {
  return StartWork(true);
}

bool Deployer::IsWorking() {
  if (!work_.valid())
    return false;
  auto status = work_.wait_for(std::chrono::milliseconds(0));
  return status != std::future_status::ready;
}

bool Deployer::IsMaintenanceMode() {
  return maintenance_mode_ && IsWorking();
}

void Deployer::JoinWorkThread() {
  if (work_.valid())
    work_.get();
}

void Deployer::JoinMaintenanceThread() {
  JoinWorkThread();
}

}  // namespace rime
