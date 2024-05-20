//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-02 GONG Chen <chen.sst@gmail.com>
//

#include <rime/build_config.h>

#ifdef RIME_ENABLE_LOGGING
#include <glog/logging.h>
#else
#include "no_logging.h"
#endif  // RIME_ENABLE_LOGGING

#include <rime_api.h>
#include <rime/deployer.h>
#include <rime/module.h>
#include <rime/service.h>
#include <rime/setup.h>

namespace rime {

#define Q(x) #x
RIME_API RIME_MODULE_LIST(kDefaultModules, "default" RIME_EXTRA_MODULES);
#undef Q
RIME_API RIME_MODULE_LIST(kDeployerModules, "deployer");
RIME_MODULE_LIST(kLegacyModules, "legacy");

RIME_REGISTER_MODULE_GROUP(default, "core", "dict", "gears")
RIME_REGISTER_MODULE_GROUP(deployer, "core", "dict", "levers")

RIME_API void LoadModules(const char* module_names[]) {
  ModuleManager& mm(ModuleManager::instance());
  for (const char** m = module_names; *m; ++m) {
    if (RimeModule* module = mm.Find(*m)) {
      mm.LoadModule(module);
    }
  }
}

// assume member is a non-null pointer in struct *p.
#define PROVIDED(p, member) \
  ((p) && RIME_STRUCT_HAS_MEMBER(*(p), (p)->member) && (p)->member)

RIME_API void SetupDeployer(RimeTraits* traits) {
  if (!traits)
    return;
  Deployer& deployer(Service::instance().deployer());
  if (PROVIDED(traits, shared_data_dir))
    deployer.shared_data_dir = path(traits->shared_data_dir);
  if (PROVIDED(traits, user_data_dir))
    deployer.user_data_dir = path(traits->user_data_dir);
  if (PROVIDED(traits, distribution_name))
    deployer.distribution_name = traits->distribution_name;
  if (PROVIDED(traits, distribution_code_name))
    deployer.distribution_code_name = traits->distribution_code_name;
  if (PROVIDED(traits, distribution_version))
    deployer.distribution_version = traits->distribution_version;
  if (PROVIDED(traits, app_name))
    deployer.app_name = traits->app_name;
  if (PROVIDED(traits, prebuilt_data_dir))
    deployer.prebuilt_data_dir = path(traits->prebuilt_data_dir);
  else
    deployer.prebuilt_data_dir = deployer.shared_data_dir / "build";
  if (PROVIDED(traits, staging_dir))
    deployer.staging_dir = path(traits->staging_dir);
  else
    deployer.staging_dir = deployer.user_data_dir / "build";
}

RIME_API void SetupLogging(const char* app_name,
                           int min_log_level,
                           const char* log_dir) {
#ifdef RIME_ENABLE_LOGGING
  FLAGS_minloglevel = min_log_level;
#ifdef RIME_ALSO_LOG_TO_STDERR
  FLAGS_alsologtostderr = true;
#endif  // RIME_ALSO_LOG_TO_STDERR
  if (log_dir) {
    if (log_dir[0] == '\0') {
      google::LogToStderr();
    } else {
      FLAGS_log_dir = log_dir;
    }
  }
  google::SetLogFilenameExtension(".log");
  google::SetLogSymlink(GLOG_INFO, app_name);
  google::SetLogSymlink(GLOG_WARNING, app_name);
  google::SetLogSymlink(GLOG_ERROR, app_name);
  // Do not allow other users to read/write log files created by current
  // process.
  FLAGS_logfile_mode = 0600;
  google::InitGoogleLogging(app_name);
#endif  // RIME_ENABLE_LOGGING
}

RIME_API void SetupLogging(const char* app_name) {
  SetupLogging(app_name, 0, NULL);
}

}  // namespace rime
