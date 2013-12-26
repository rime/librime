//
// Copyleft RIME Developers
// License: GPLv3
//
// 2012-03-23 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_USER_DICT_MANAGER_H_
#define RIME_USER_DICT_MANAGER_H_

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace rime {

class Deployer;

using UserDictList = std::vector<std::string>;

class UserDictManager {
 public:
  UserDictManager(Deployer* deployer);

  void GetUserDictList(UserDictList* user_dict_list);

  // CAVEAT: the user dict should be closed before the following operations
  bool Backup(const std::string& dict_name);
  bool Restore(const std::string& snapshot_file);
  bool UpgradeUserDict(const std::string& dict_name);
  // returns num of exported entires, -1 denotes failure
  int Export(const std::string& dict_name, const std::string& text_file);
  // returns num of imported entires, -1 denotes failure
  int Import(const std::string& dict_name, const std::string& text_file);

  bool Synchronize(const std::string& dict_name);
  bool SynchronizeAll();

 protected:
  Deployer* deployer_;
  boost::filesystem::path path_;
};

}  // namespace rime

#endif  // RIME_USER_DICT_MANAGER_H_
