//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-10 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_DEPLOYMENT_TASKS_H_
#define RIME_DEPLOYMENT_TASKS_H_

#include <rime/deployer.h>

namespace rime {

// initialize/update installation.yaml, default.yaml
class InstallationUpdate : public DeploymentTask {
 public:
  InstallationUpdate(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

// update distributed config files and preset schemas
class WorkspaceUpdate : public DeploymentTask {
 public:
  WorkspaceUpdate(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);

 protected:
  string GetSchemaPath(Deployer* deployer,
                            const string& schema_id,
                            bool prefer_shared_copy);
};

// update a specific schema, build corresponding dictionary
class SchemaUpdate : public DeploymentTask {
 public:
  explicit SchemaUpdate(const string& schema_file)
      : schema_file_(schema_file) {}
  SchemaUpdate(TaskInitializer arg);
  bool Run(Deployer* deployer);
  void set_verbose(bool verbose) { verbose_ = verbose; }

 protected:
  string schema_file_;
  bool verbose_ = false;
};

// update a specific config file
class ConfigFileUpdate : public DeploymentTask {
 public:
  ConfigFileUpdate(const string& file_name,
                   const string& version_key)
      : file_name_(file_name), version_key_(version_key) {}
  ConfigFileUpdate(TaskInitializer arg);
  bool Run(Deployer* deployer);

 protected:
  string file_name_;
  string version_key_;
};

// for installer
class PrebuildAllSchemas : public DeploymentTask {
 public:
  PrebuildAllSchemas(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

// create symlinks to prebuilt dictionaries in user directory
class SymlinkingPrebuiltDictionaries : public DeploymentTask {
 public:
  SymlinkingPrebuiltDictionaries(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

// upgrade user dictionaries
class UserDictUpgrade : public DeploymentTask {
 public:
  UserDictUpgrade(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

class UserDictSync : public DeploymentTask {
 public:
  UserDictSync(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

class BackupConfigFiles : public DeploymentTask {
 public:
  BackupConfigFiles(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

class CleanupTrash : public DeploymentTask {
 public:
  CleanupTrash(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

class CleanOldLogFiles : public DeploymentTask {
 public:
  CleanOldLogFiles(TaskInitializer arg = TaskInitializer()) {}
  bool Run(Deployer* deployer);
};

}  // namespace rime

#endif  // RIME_DEPLOYMENT_TASKS_H_
