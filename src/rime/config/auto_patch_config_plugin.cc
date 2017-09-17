//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <boost/algorithm/string.hpp>
#include <rime/config/config_compiler_impl.h>
#include <rime/config/plugins.h>

namespace rime {

static string remove_suffix(const string& input, const string& suffix) {
  return boost::ends_with(input, suffix) ?
      input.substr(0, input.length() - suffix.length()) : input;
}

// auto-patch applies to all loaded config resources, including dependencies.
// therefore it's done at the end of Compile phase.
bool AutoPatchConfigPlugin::ReviewCompileOutput(ConfigCompiler* compiler,
                                                an<ConfigResource> resource) {
  if (boost::ends_with(resource->resource_id, ".custom"))
    return true;
  // skip auto-patch if there is already an explicit `__patch` at the root node
  auto root_deps = compiler->GetDependencies(resource->resource_id + ":");
  if (!root_deps.empty() && root_deps.back()->priority() >= kPatch)
    return true;
  auto patch_resource_id =
      remove_suffix(resource->resource_id, ".schema") + ".custom";
  LOG(INFO) << "auto-patch " << resource->resource_id << ":/__patch: "
            << patch_resource_id << ":/patch?";
  compiler->Push(resource);
  compiler->AddDependency(
      New<PatchReference>(Reference{patch_resource_id, "patch", true}));
  compiler->Pop();
  return true;
}

bool AutoPatchConfigPlugin::ReviewLinkOutput(ConfigCompiler* compiler,
                                             an<ConfigResource> resource) {
  return true;
}

}  // namespace rime
