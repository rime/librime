//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-03-24 GONG Chen <chen.sst@gmail.com>
//
#include <iostream>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/service.h>
#include <rime/setup.h>
#include <rime/dict/level_db.h>
#include <rime/dict/user_db.h>
#include <rime/lever/user_dict_manager.h>

int main(int argc, char *argv[]) {
  rime::SetupLogging("rime.tools");

  rime::string option;
  rime::string arg1, arg2;
  if (argc >= 2) option = argv[1];
  if (argc >= 3) arg1 = argv[2];
  if (argc >= 4) arg2 = argv[3];
  if (argc == 1) {
    std::cout << "options:" << std::endl
              << "\t-l|--list" << std::endl
              << "\t-s|--sync" << std::endl
              << "\t-b|--backup dict_name" << std::endl
              << "\t-r|--restore xxx.userdb.txt" << std::endl
              << "\t-e|--export dict_name export.txt" << std::endl
              << "\t-i|--import dict_name import.txt" << std::endl
        ;
    return 0;
  }

  rime::Registry& registry = rime::Registry::instance();
  registry.Register("userdb", new rime::UserDbComponent<rime::LevelDb>);
  rime::Deployer& deployer(rime::Service::instance().deployer());
  {
    rime::Config config;
    if (config.LoadFromFile("installation.yaml")) {
      config.GetString("installation_id", &deployer.user_id);
      config.GetString("sync_dir", &deployer.sync_dir);
    }
  }
  rime::UserDictManager mgr(&deployer);
  if (argc == 2 && (option == "-l" || option == "--list")) {
    rime::UserDictList list;
    mgr.GetUserDictList(&list);
    if (list.empty()) {
      std::cerr << "no user dictionary is found." << std::endl;
      return 0;
    }
    for (const rime::string& e : list) {
      std::cout << e << std::endl;
    }
    return 0;
  }
  if (argc == 2 && (option == "-s" || option == "--sync")) {
    std::cout << "sync dir: " << deployer.sync_dir << std::endl;
    std::cout << "user id: " << deployer.user_id << std::endl;
    if (mgr.SynchronizeAll())
      return 0;
    else
      return 1;
  }
  if (argc == 3 && (option == "-b" || option == "--backup")) {
    if (mgr.Backup(arg1))
      return 0;
    else
      return 1;
  }
  if (argc == 3 && (option == "-r" || option == "--restore")) {
    if (mgr.Restore(arg1))
      return 0;
    else
      return 1;
  }
  if (argc == 4 && (option == "-e" || option == "--export")) {
    int n = mgr.Export(arg1, arg2);
    if (n == -1) return 1;
    std::cout << "exported " << n << " entries." << std::endl;
    return 0;
  }
  if (argc == 4 && (option == "-i" || option == "--import")) {
    int n = mgr.Import(arg1, arg2);
    if (n == -1) return 1;
    std::cout << "imported " << n << " entries." << std::endl;
    return 0;
  }
  std::cerr << "invalid arguments." << std::endl;
  return 1;
}
