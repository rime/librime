//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SETUP_H_
#define RIME_SETUP_H_

#include <functional>

#include <rime_api.h>

namespace rime {

using LogHandler = std::function<void(const RimeLogRecord& record)>;

RIME_DLL extern const char* kDefaultModules[];
RIME_DLL extern const char* kDeployerModules[];
extern const char* kLegacyModules[];

RIME_DLL void LoadModules(const char* module_names[]);

RIME_DLL void SetupDeployer(RimeTraits* traits);

RIME_DLL void SetupLogging(const char* app_name,
                           int min_log_level,
                           const char* log_dir);
RIME_DLL void SetupLogging(const char* app_name);

//! Install a global log handler. An empty handler disables log output.
//! Replacing or clearing the handler waits for callbacks that are already
//! running to finish, so captured state can be destroyed after it returns.
//! The handler is called synchronously on the logging thread; calls are
//! serialized by an internal lock. It must not call SetLogHandler,
//! ClearLogHandler, or emit librime log records (this would deadlock).
//! Only effective when librime is built with ENABLE_LOGGING=OFF; with
//! ENABLE_LOGGING=ON, google-glog handles logging and these functions
//! return false without installing or clearing a handler.
//! Returns true when the handler was installed or cleared.
RIME_DLL bool SetLogHandler(LogHandler handler);
RIME_DLL bool ClearLogHandler();

}  // namespace rime

#endif  // RIME_SETUP_H_
