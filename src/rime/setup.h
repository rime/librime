//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SETUP_H_
#define RIME_SETUP_H_

#include <rime_api.h>

namespace rime {

RIME_DLL extern const char* kDefaultModules[];
RIME_DLL extern const char* kDeployerModules[];
extern const char* kLegacyModules[];

RIME_DLL void LoadModules(const char* module_names[]);

RIME_DLL void SetupDeployer(RimeTraits* traits);

RIME_DLL void SetupLogging(const char* app_name,
                           int min_log_level,
                           const char* log_dir);
RIME_DLL void SetupLogging(const char* app_name);

//! Install a global log handler. NULL disables log output.
//! Only effective when librime is built with ENABLE_LOGGING=OFF; with
//! ENABLE_LOGGING=ON, google-glog handles logging and this is a no-op.
RIME_DLL void SetLogHandler(RimeLogHandler handler, void* context_object);

}  // namespace rime

#endif  // RIME_SETUP_H_
