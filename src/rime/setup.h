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

//! Install a global log handler. NULL disables log output.
RIME_DLL void SetLogHandler(RimeLogHandler handler, void* context_object);

}  // namespace rime

#endif  // RIME_SETUP_H_
