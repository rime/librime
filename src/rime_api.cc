//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-09 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <sstream>
#include <rime/common.h>
#include <rime/module.h>
#include <rime/setup.h>

#include "rime_api_impl.h"

using namespace rime;

RIME_DEPRECATED void RimeSetupLogging(const char* app_name) {
  SetupLogging(app_name);
}

#if RIME_BUILD_SHARED_LIBS
void rime_declare_module_dependencies() {}
#else
extern void rime_require_module_core();
extern void rime_require_module_dict();
extern void rime_require_module_gears();
extern void rime_require_module_levers();
// link to default modules explicitly when building static library.
void rime_declare_module_dependencies() {
  rime_require_module_core();
  rime_require_module_dict();
  rime_require_module_gears();
  rime_require_module_levers();
}
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
