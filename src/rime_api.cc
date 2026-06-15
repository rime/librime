//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-09 GONG Chen <chen.sst@gmail.com>
//

#include "rime_api_impl.h"

#include <cstring>
#include <sstream>
#include <rime/common.h>
#include <rime/module.h>
#include <rime/setup.h>

using namespace rime;

RIME_DEPRECATED void RimeSetupLogging(const char* app_name) {
  SetupLogging(app_name);
}

#if RIME_BUILD_SHARED_LIBS
void rime_declare_module_dependencies() {}
#else
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/vmd/is_empty.hpp>

#define _RIME_SEQ_FOR_EACH(macro, data, seq)                   \
  BOOST_PP_IIF(BOOST_VMD_IS_EMPTY(seq), BOOST_PP_TUPLE_EAT(3), \
               BOOST_PP_SEQ_FOR_EACH)                          \
  (macro, data, seq)

extern void rime_require_module_core();
extern void rime_require_module_dict();
extern void rime_require_module_gears();
extern void rime_require_module_levers();

#define _RIME_PLUGIN_DECL(r, data, elem) \
  extern void BOOST_PP_CAT(rime_require_module_, elem)();
#define _RIME_PLUGIN_CALL(r, data, elem) \
  BOOST_PP_CAT(rime_require_module_, elem)();

_RIME_SEQ_FOR_EACH(_RIME_PLUGIN_DECL, ~, RIME_EXTRA_MODULES)

// link to default modules explicitly when building static library.
void rime_declare_module_dependencies() {
  rime_require_module_core();
  rime_require_module_dict();
  rime_require_module_gears();
  rime_require_module_levers();
  _RIME_SEQ_FOR_EACH(_RIME_PLUGIN_CALL, ~, RIME_EXTRA_MODULES)
}

#undef _RIME_PLUGIN_DECL
#undef _RIME_PLUGIN_CALL
#undef _RIME_SEQ_FOR_EACH
#undef RIME_EXTRA_MODULES
#endif

RIME_API Bool RimeRegisterModule(RimeModule* module) {
  if (!module || !module->module_name)
    return False;
  ModuleManager::instance().Register(module->module_name, module);
  return True;
}

RIME_API RimeModule* RimeFindModule(const char* module_name) {
  return ModuleManager::instance().Find(module_name);
}

void RimeGetSharedDataDirSecure(char* dir, size_t buffer_size) {
  string string_path = Service::instance().deployer().shared_data_dir.string();
  strncpy(dir, string_path.c_str(), buffer_size);
}

void RimeGetUserDataDirSecure(char* dir, size_t buffer_size) {
  string string_path = Service::instance().deployer().user_data_dir.string();
  strncpy(dir, string_path.c_str(), buffer_size);
}

void RimeGetPrebuiltDataDirSecure(char* dir, size_t buffer_size) {
  string string_path =
      Service::instance().deployer().prebuilt_data_dir.string();
  strncpy(dir, string_path.c_str(), buffer_size);
}

void RimeGetStagingDirSecure(char* dir, size_t buffer_size) {
  string string_path = Service::instance().deployer().staging_dir.string();
  strncpy(dir, string_path.c_str(), buffer_size);
}

void RimeGetSyncDirSecure(char* dir, size_t buffer_size) {
  string string_path = Service::instance().deployer().sync_dir.string();
  strncpy(dir, string_path.c_str(), buffer_size);
}

const char* RimeGetVersion() {
  return RIME_VERSION;
}
