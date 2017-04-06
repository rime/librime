//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <rime/config/config_data.h>
#include <rime/config/config_data_manager.h>

namespace rime {

ConfigDataManager& ConfigDataManager::instance() {
  static the<ConfigDataManager> s_instance;
  if (!s_instance) {
    s_instance.reset(new ConfigDataManager);
  }
  return *s_instance;
}

an<ConfigData>
ConfigDataManager::GetConfigData(const string& config_file_path) {
  an<ConfigData> sp;
  // keep a weak reference to the shared config data in the manager
  weak<ConfigData>& wp((*this)[config_file_path]);
  if (wp.expired()) {  // create a new copy and load it
    sp = New<ConfigData>();
    sp->LoadFromFile(config_file_path);
    wp = sp;
  }
  else {  // obtain the shared copy
    sp = wp.lock();
  }
  return sp;
}

bool ConfigDataManager::ReloadConfigData(const string& config_file_path) {
  iterator it = find(config_file_path);
  if (it == end()) {  // never loaded
    return false;
  }
  an<ConfigData> sp = it->second.lock();
  if (!sp)  {  // already been freed
    erase(it);
    return false;
  }
  sp->LoadFromFile(config_file_path);
  return true;
}

}  // namespace rime
