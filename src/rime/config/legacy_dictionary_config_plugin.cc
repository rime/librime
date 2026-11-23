//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/config/config_compiler_impl.h>
#include <rime/config/plugins.h>

namespace rime {

bool LegacyDictionaryConfigPlugin::ReviewCompileOutput(ConfigCompiler*,
                                                       an<ConfigResource>) {
  // TODO: unimplemented
  return true;
}

bool LegacyDictionaryConfigPlugin::ReviewLinkOutput(ConfigCompiler*,
                                                    an<ConfigResource>) {
  // TODO: unimplemented
  return true;
}

}  // namespace rime
