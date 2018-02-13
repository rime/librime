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

SaveOutputPlugin::SaveOutputPlugin(const ResourceType& output_resource)
    : resource_resolver_(new ResourceResolver(output_resource)) {
  resource_resolver_->set_root_path(
      Service::instance().deployer().user_data_dir);
}

SaveOutputPlugin::~SaveOutputPlugin() {}

bool SaveOutputPlugin::ReviewCompileOutput(
    ConfigCompiler* compiler, an<ConfigResource> resource) {
  return true;
}

bool SaveOutputPlugin::ReviewLinkOutput(
    ConfigCompiler* compiler, an<ConfigResource> resource) {
  auto file_path = resource_resolver_->ResolvePath(resource->resource_id);
  return resource->data->SaveToFile(file_path.string());
}

}  // namespace rime
