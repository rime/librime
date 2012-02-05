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
#include <boost/thread.hpp>

namespace rime {

struct Deployer {
  std::string shared_data_dir;
  std::string user_data_dir;
  std::string user_id;
  
  std::string distribution_name;
  std::string distribution_code_name;
  std::string distribution_version;

  boost::thread maintenance_thread;

  Deployer() : shared_data_dir("."),
               user_data_dir("."),
               user_id("unknown") {}
  
  bool InitializeInstallation();
  bool InstallSchema(const std::string &schema_file);
  bool UpdateDistributedConfigFile(const std::string &file_name,
                                   const std::string &version_key);
  bool PrepareWorkspace();
  bool StartMaintenance(bool thorough_check = true);
  void JoinMaintenanceThread();
  bool IsMaintenancing();
};

}  // namespace rime

#endif  // RIME_DEPLOYER_H_
