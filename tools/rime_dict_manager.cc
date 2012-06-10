// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-03-24 GONG Chen <chen.sst@gmail.com>
//
#include <iostream>
#include <string>
#include <boost/foreach.hpp>
#include <rime/deployer.h>
#include <rime/expl/user_dict_manager.h>

int main(int argc, char *argv[]) {
  rime::Deployer deployer;
  rime::UserDictManager mgr(&deployer);
  std::string option;
  std::string arg1, arg2;
  if (argc >= 2) option = argv[1];
  if (argc >= 3) arg1 = argv[2];
  if (argc >= 4) arg2 = argv[3];
  if (argc == 1) {
    std::cout << "options:" << std::endl
              << "\t-l|--list" << std::endl
              << "\t-b|--backup dict_name" << std::endl
              << "\t-r|--restore xxx.userdb.kct.snapshot" << std::endl
              << "\t-e|--export dict_name export.txt" << std::endl
              << "\t-i|--import dict_name import.txt" << std::endl
        ;
    return 0;
  }
  if (argc == 2 && (option == "-l" || option == "--list")) {
    rime::UserDictList list;
    mgr.GetUserDictList(&list);
    if (list.empty()) {
      std::cerr << "no user dictionary is found." << std::endl;
      return 0;
    }
    BOOST_FOREACH(const std::string&e, list) {
      std::cout << e << std::endl;
    }
    return 0;
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
