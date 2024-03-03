//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/dll.hpp>
#include <filesystem>
#include <rime/build_config.h>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/module.h>
#include <rime/registry.h>
#include <rime_api.h>

namespace fs = std::filesystem;

namespace rime {

class PluginManager {
 public:
  void LoadPlugins(path plugins_dir);

  static string plugin_name_of(path plugin_file);

  static PluginManager& instance();

 private:
  PluginManager() = default;

  map<string, boost::dll::shared_library> plugin_libs_;
};

void PluginManager::LoadPlugins(path plugins_dir) {
  ModuleManager& mm = ModuleManager::instance();
  if (!fs::is_directory(plugins_dir)) {
    return;
  }
  LOG(INFO) << "loading plugins from " << plugins_dir;
  for (fs::directory_iterator iter(plugins_dir), end; iter != end; ++iter) {
    path plugin_file = iter->path();
    if (plugin_file.extension() == boost::dll::shared_library::suffix()) {
      fs::file_status plugin_file_status = fs::status(plugin_file);
      if (fs::is_regular_file(plugin_file_status)) {
        DLOG(INFO) << "found plugin: " << plugin_file;
        string plugin_name = plugin_name_of(plugin_file);
        if (plugin_libs_.find(plugin_name) == plugin_libs_.end()) {
          LOG(INFO) << "loading plugin '" << plugin_name << "' from "
                    << plugin_file;
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

string PluginManager::plugin_name_of(path plugin_file) {
  string name = plugin_file.stem().string();
  // remove prefix "(lib)rime-"
  if (boost::starts_with(name, "librime-")) {
    boost::erase_first(name, "librime-");
  } else if (boost::starts_with(name, "rime-")) {
    boost::erase_first(name, "rime-");
  }
  // replace dash with underscore, for the plugin name is part of the module
  // initializer function name.
  std::replace(name.begin(), name.end(), '-', '_');
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

#ifdef _WIN32
// TODO: implement this when ready to support DLL plugins on Windows.
inline static rime::path current_module_path() {
  return rime::path{};
}
#else
#include <dlfcn.h>

inline static rime::path symbol_location(const void* symbol) {
  Dl_info info;
  // Some of the libc headers miss `const` in `dladdr(const void*, Dl_info*)`
  const int res = dladdr(const_cast<void*>(symbol), &info);
  if (res) {
    return rime::path{info.dli_fname};
  } else {
    return rime::path{};
  }
}

inline static rime::path current_module_path() {
  void rime_require_module_plugins();
  return symbol_location(
      reinterpret_cast<const void*>(&rime_require_module_plugins));
}
#endif

static void rime_plugins_initialize() {
  rime::PluginManager::instance().LoadPlugins(
      current_module_path().remove_filename() / RIME_PLUGINS_DIR);
}

static void rime_plugins_finalize() {}

RIME_REGISTER_MODULE(plugins)
