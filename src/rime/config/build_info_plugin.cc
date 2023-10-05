//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/fs.h>
#include <rime/service.h>
#include <rime/config/config_compiler.h>
#include <rime/config/config_types.h>
#include <rime/config/plugins.h>

namespace rime {

bool BuildInfoPlugin::ReviewCompileOutput(ConfigCompiler* compiler,
                                          an<ConfigResource> resource) {
  return true;
}

bool BuildInfoPlugin::ReviewLinkOutput(ConfigCompiler* compiler,
                                       an<ConfigResource> resource) {
  auto build_info = (*resource)["__build_info"];
  build_info["rime_version"] = RIME_VERSION;
#ifndef RIME_NO_TIMESTAMP
  auto timestamps = build_info["timestamps"];
  compiler->EnumerateResources([&](an<ConfigResource> resource) {
    if (!resource->loaded) {
      LOG(INFO) << "resource '" << resource->resource_id << "' not loaded.";
      timestamps[resource->resource_id] = 0;
      return;
    }
    auto file_name = resource->data->file_name();
    if (file_name.empty()) {
      LOG(WARNING) << "resource '" << resource->resource_id
                   << "' is not persisted.";
      timestamps[resource->resource_id] = 0;
      return;
    }
    // TODO: store as 64-bit number to avoid the year 2038 problem
    std::time_t ft = fs::last_write_time(file_name);
    timestamps[resource->resource_id] = (int) ft;
  });
#endif
  return true;
}

}  // namespace rime
