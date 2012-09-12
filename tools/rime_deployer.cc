// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-07-07 GONG Chen <chen.sst@gmail.com>
//
#include <iostream>
#include <string>
#include <rime/deployer.h>
#include <rime/service.h>
#include <rime/lever/deployment_tasks.h>

int main(int argc, char *argv[]) {
  rime::SetupLogging("rime.tools");

  std::string option;
  std::string arg1, arg2;
  if (argc >= 2) option = argv[1];
  if (argc >= 3) arg1 = argv[2];
  if (argc >= 4) arg2 = argv[3];
  if (argc == 1) {
    std::cout << "options:" << std::endl
              << "\t--build [dest_dir [source_dir]]" << std::endl
        ;
    return 0;
  }
  rime::Deployer& deployer(rime::Service::instance().deployer());
  if (argc >= 2 && argc <= 4 && option == "--build") {
    if (!arg1.empty()) {
      deployer.user_data_dir = arg1;
      if (!arg2.empty()) {
        deployer.shared_data_dir = arg2;
      }
      else {
        deployer.shared_data_dir = arg1;
      }
    }
    rime::WorkspaceUpdate update;
    return update.Run(&deployer) ? 0 : 1;
  }
  std::cerr << "invalid arguments." << std::endl;
  return 1;
}
