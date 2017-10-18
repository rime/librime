//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_CONFIG_PLUGINS_H_
#define RIME_CONFIG_PLUGINS_H_

#include <rime/common.h>

namespace rime {

class ConfigCompiler;
struct ConfigResource;

class ConfigCompilerPlugin {
 public:
  typedef bool Review(ConfigCompiler* compiler,
                      an<ConfigResource> resource);

  virtual Review ReviewCompileOutput = 0;
  virtual Review ReviewLinkOutput = 0;
};

class AutoPatchConfigPlugin : public ConfigCompilerPlugin {
 public:
  Review ReviewCompileOutput;
  Review ReviewLinkOutput;
};

class DefaultConfigPlugin : public ConfigCompilerPlugin {
 public:
  Review ReviewCompileOutput;
  Review ReviewLinkOutput;
};

class LegacyPresetConfigPlugin : public ConfigCompilerPlugin {
 public:
  Review ReviewCompileOutput;
  Review ReviewLinkOutput;
};

class LegacyDictionaryConfigPlugin : public ConfigCompilerPlugin {
 public:
  Review ReviewCompileOutput;
  Review ReviewLinkOutput;
};

}  // namespace rime

#endif  // RIME_CONFIG_PLUGINS_H_
