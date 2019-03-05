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

  virtual ~ConfigCompilerPlugin() = default;

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

class BuildInfoPlugin : public ConfigCompilerPlugin {
 public:
  Review ReviewCompileOutput;
  Review ReviewLinkOutput;
};

class ResourceResolver;
struct ResourceType;

class SaveOutputPlugin : public ConfigCompilerPlugin {
 public:
  SaveOutputPlugin(const ResourceType& output_resource);
  virtual ~SaveOutputPlugin();

  Review ReviewCompileOutput;
  Review ReviewLinkOutput;

 private:
  the<ResourceResolver> resource_resolver_;
};

}  // namespace rime

#endif  // RIME_CONFIG_PLUGINS_H_
