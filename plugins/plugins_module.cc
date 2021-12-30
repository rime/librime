//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <rime/build_config.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/module.h>
#include <rime/registry.h>
#include <rime_api.h>

namespace fs = boost::filesystem;

namespace rime {

class PluginManager {
 public:
  void LoadPlugins(fs::path plugins_dir);

  static string plugin_name_of(fs::path plugin_file);

  static PluginManager& instance();

 private:
  PluginManager() = default;

  map<string, boost::dll::shared_library> plugin_libs_;
};

void PluginManager::LoadPlugins(fs::path plugins_dir) {
  ModuleManager& mm = ModuleManager::instance();
  if (!fs::is_directory(plugins_dir)) {
    return;
  }
  LOG(INFO) << "loading plugins from " << plugins_dir;
  for (fs::directory_iterator iter(plugins_dir), end; iter != end; ++iter) {
    fs::path plugin_file = iter->path();
    if (plugin_file.extension() == boost::dll::shared_library::suffix()) {
      fs::file_status plugin_file_status = fs::status(plugin_file);
      if (fs::is_regular_file(plugin_file_status)) {
        DLOG(INFO) << "found plugin: " << plugin_file;
        string plugin_name = plugin_name_of(plugin_file);
        if (plugin_libs_.find(plugin_name) == plugin_libs_.end()) {
          LOG(INFO) << "loading plugin '" << plugin_name
                    << "' from " << plugin_file;
          try {
            auto plugin_lib = boost::dll::shared_library(plugin_file);
            plugin_libs_[plugin_name] = plugin_lib;
          } catch (const std::exception& ex) {
            LOG(ERROR) << "error loading plugin " << plugin_name << ": "
                       << ex.what();
            continue;
          }
        }
        if (RimeModule* module = mm.Find(plugin_name)) {
          mm.LoadModule(module);
          LOG(INFO) << "loaded plugin: " << plugin_name;
        } else {
          LOG(WARNING) << "module '" << plugin_name
                       << "' is not provided by plugin library " << plugin_file;
        }
      }
    }
  }
}

string PluginManager::plugin_name_of(fs::path plugin_file) {
  string name = plugin_file.stem().string();
  // remove prefix "(lib)rime-"
  if (boost::starts_with(name, "librime-")) {
    boost::erase_first(name, "librime-");
  } else if (boost::starts_with(name, "rime-")) {
    boost::erase_first(name, "rime-");
  }
  // replace dash with underscore, for the plugin name is part of the module
  // initializer function name.
  boost::replace_all(name, "-", "_");
  return name;
}

PluginManager& PluginManager::instance() {
  static the<PluginManager> s_instance;
  if (!s_instance) {
    s_instance.reset(new PluginManager);
  }
  return *s_instance;
}

}  // namespace rime

static void rime_plugins_initialize() {
  rime::PluginManager::instance().LoadPlugins(RIME_PLUGINS_DIR);
}

static void rime_plugins_finalize() {}

RIME_REGISTER_MODULE(plugins)
