//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/config/config_compiler_impl.h>
#include <rime/config/plugins.h>

namespace rime {

bool LegacyDictionaryConfigPlugin::ReviewCompileOutput(
    ConfigCompiler* compiler, an<ConfigResource> resource) {
  // TODO: unimplemented
  return true;
}

bool LegacyDictionaryConfigPlugin::ReviewLinkOutput(
    ConfigCompiler* compiler, an<ConfigResource> resource) {
  // TODO: unimplemented
  return true;
}

}  // namespace rime
