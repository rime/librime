//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <filesystem>
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

SaveOutputPlugin::SaveOutputPlugin(const string& output_dir):resource_resolver_(new ResourceResolver(kCompiledConfig)) {
  resource_resolver_->set_root_path(path(output_dir));
}

SaveOutputPlugin::~SaveOutputPlugin() {}

bool SaveOutputPlugin::ReviewCompileOutput(ConfigCompiler* compiler,
                                           an<ConfigResource> resource) {
  return true;
}

bool SaveOutputPlugin::ReviewLinkOutput(ConfigCompiler* compiler,
                                        an<ConfigResource> resource) {
  LOG(INFO) << "SaveOutputPlugin::ReviewLinkOutput for resource: " 
            << resource->resource_id;
  
  auto file_path = resource_resolver_->ResolvePath(resource->resource_id);
  LOG(INFO) << "Attempting to save to: " << file_path;
  
  // ensure directory exists
  auto parent_dir = file_path.parent_path();
  if (!std::filesystem::exists(parent_dir)) {
    LOG(INFO) << "Creating directory: " << parent_dir;
    try {
      std::filesystem::create_directories(parent_dir);
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to create directory " << parent_dir << ": " << e.what();
      return false;
    }
  }
  
  bool result = resource->data->SaveToFile(file_path);
  if (result) {
    LOG(INFO) << "Successfully saved config to: " << file_path;
  } else {
    LOG(ERROR) << "Failed to save config to: " << file_path;
  }
  
  return result;
}

}  // namespace rime
