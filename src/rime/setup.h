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

RIME_API extern const char* kDefaultModules[];
extern const char* kDeployerModules[];
extern const char* kLegacyModules[];

RIME_API void LoadModules(const char* module_names[]);

RIME_API void SetupLogging(const char* app_name);

}  // namespace rime

#endif  // RIME_SETUP_H_
