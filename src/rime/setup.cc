//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-10-02 GONG Chen <chen.sst@gmail.com>
//

#include <rime/build_config.h>

#ifdef RIME_ENABLE_LOGGING
#include "logging.h"
#else
#include "no_logging.h"
#endif  // RIME_ENABLE_LOGGING

#include <rime_api.h>
#include <rime/deployer.h>
#include <rime/module.h>
#include <rime/service.h>
#include <rime/setup.h>

namespace rime {

#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/vmd/is_empty.hpp>

#define _RIME_SEQ_FOR_EACH(macro, data, seq)                   \
  BOOST_PP_IIF(BOOST_VMD_IS_EMPTY(seq), BOOST_PP_TUPLE_EAT(3), \
               BOOST_PP_SEQ_FOR_EACH)                          \
  (macro, data, seq)

#define _RIME_MODULE_STR(r, data, elem) , BOOST_PP_STRINGIZE(elem)
RIME_DLL RIME_MODULE_LIST(kDefaultModules,
                          "default" _RIME_SEQ_FOR_EACH(_RIME_MODULE_STR,
                                                       ~,
                                                       RIME_EXTRA_MODULES));
#undef _RIME_MODULE_STR
#undef _RIME_SEQ_FOR_EACH
RIME_DLL RIME_MODULE_LIST(kDeployerModules, "deployer");
RIME_MODULE_LIST(kLegacyModules, "legacy");

RIME_REGISTER_MODULE_GROUP(default, "core", "dict", "gears")
RIME_REGISTER_MODULE_GROUP(deployer, "core", "dict", "levers")

RIME_DLL void LoadModules(const char* module_names[]) {
  ModuleManager& mm(ModuleManager::instance());
  for (const char** m = module_names; *m; ++m) {
    if (RimeModule* module = mm.Find(*m)) {
      mm.LoadModule(module);
    }
  }
}

RIME_DLL void SetupDeployer(RimeTraits* traits) {
  if (!traits)
    return;
  Deployer& deployer(Service::instance().deployer());
  if (RIME_PROVIDED(traits, shared_data_dir))
    deployer.shared_data_dir = path(traits->shared_data_dir);
  if (RIME_PROVIDED(traits, user_data_dir))
    deployer.user_data_dir = path(traits->user_data_dir);
  if (RIME_PROVIDED(traits, distribution_name))
    deployer.distribution_name = traits->distribution_name;
  if (RIME_PROVIDED(traits, distribution_code_name))
    deployer.distribution_code_name = traits->distribution_code_name;
  if (RIME_PROVIDED(traits, distribution_version))
    deployer.distribution_version = traits->distribution_version;
  if (RIME_PROVIDED(traits, app_name))
    deployer.app_name = traits->app_name;
  if (RIME_PROVIDED(traits, prebuilt_data_dir))
    deployer.prebuilt_data_dir = path(traits->prebuilt_data_dir);
  else
    deployer.prebuilt_data_dir = deployer.shared_data_dir / "build";
  if (RIME_PROVIDED(traits, staging_dir))
    deployer.staging_dir = path(traits->staging_dir);
  else
    deployer.staging_dir = deployer.user_data_dir / "build";
}

RIME_DLL void SetupLogging(const char* app_name,
                           int min_log_level,
                           const char* log_dir) {
#ifdef RIME_ENABLE_LOGGING
  InitializeLogging(app_name, min_log_level, log_dir);
#endif  // RIME_ENABLE_LOGGING
}

RIME_DLL void SetupLogging(const char* app_name) {
  SetupLogging(app_name, 0, NULL);
}

}  // namespace rime
