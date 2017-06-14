//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <rime/common.h>
#include <rime/config/config_compiler.h>
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
  // keep a weak reference to the shared config data in the manager
  weak<ConfigData>& wp((*this)[config_file_path]);
  if (wp.expired()) {  // create a new copy and load it
    ConfigCompiler compiler;
    auto resource = compiler.Compile(config_file_path);
    if (!resource || !compiler.Link(resource)) {
      LOG(ERROR) << "error loading config from " << config_file_path;
    }
    if (!resource) {
      return New<ConfigData>();
    }
    wp = resource->data;
    return resource->data;
  }
  // obtain the shared copy
  return wp.lock();
}

}  // namespace rime
