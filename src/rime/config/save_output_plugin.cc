//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <boost/filesystem.hpp>
#include <rime/resource.h>
#include <rime/service.h>
#include <rime/config/config_compiler.h>
#include <rime/config/config_types.h>
#include <rime/config/plugins.h>

namespace rime {

static const ResourceType kCompiledConfig = {"compiled_config", "", ".yaml"};

SaveOutputPlugin::SaveOutputPlugin()
    : resource_resolver_(
          Service::instance().CreateStagingResourceResolver(kCompiledConfig)) {}

SaveOutputPlugin::~SaveOutputPlugin() {}

bool SaveOutputPlugin::ReviewCompileOutput(ConfigCompiler* compiler,
                                           an<ConfigResource> resource) {
  return true;
}

bool SaveOutputPlugin::ReviewLinkOutput(ConfigCompiler* compiler,
                                        an<ConfigResource> resource) {
  auto file_path = resource_resolver_->ResolvePath(resource->resource_id);
  return resource->data->SaveToFile(file_path.string());
}

}  // namespace rime
