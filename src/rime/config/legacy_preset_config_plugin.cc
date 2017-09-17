//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <boost/algorithm/string.hpp>
#include <rime/config/config_compiler_impl.h>
#include <rime/config/config_cow_ref.h>
#include <rime/config/plugins.h>

namespace rime {

bool LegacyPresetConfigPlugin::ReviewCompileOutput(
    ConfigCompiler* compiler, an<ConfigResource> resource) {
  return true;
}

bool LegacyPresetConfigPlugin::ReviewLinkOutput(
    ConfigCompiler* compiler, an<ConfigResource> resource) {
  if (!boost::ends_with(resource->resource_id, ".schema"))
    return true;
  if (auto preset = resource->data->Traverse("key_binder/import_preset")) {
    if (!Is<ConfigValue>(preset))
      return false;
    auto preset_config_id = As<ConfigValue>(preset)->str();
    LOG(INFO) << "interpreting key_binder/import_preset: " << preset_config_id;
    auto target = Cow(resource, "key_binder");
    auto map = As<ConfigMap>(**target);
    if (map && map->HasKey("bindings")) {
      // append to included list `key_binder/bindings/+` instead of overwriting
      auto appended = map->Get("bindings");
      *Cow(target, "bindings/+") = appended;
      // `*target` is already referencing a copied map, safe to edit directly
      (*target)["bindings"] = nullptr;
    }
    Reference reference{preset_config_id, "key_binder", false};
    if (!IncludeReference{reference}
        .TargetedAt(target).Resolve(compiler)) {
      LOG(ERROR) << "failed to include section " << reference;
      return false;
    }
  }
  // NOTE: in the following cases, Cow() is not strictly necessary because
  // we know for sure that no other ConfigResource is going to reference the
  // root map node that will be modified. But other than the root node of the
  // resource being linked, it's possbile a map or list has multiple references
  // in the node tree, therefore Cow() is recommended to make sure the
  // modifications only happen to one place.
  if (auto preset = resource->data->Traverse("punctuator/import_preset")) {
    if (!Is<ConfigValue>(preset))
      return false;
    auto preset_config_id = As<ConfigValue>(preset)->str();
    LOG(INFO) << "interpreting punctuator/import_preset: " << preset_config_id;
    Reference reference{preset_config_id, "punctuator", false};
    if (!IncludeReference{reference}
        .TargetedAt(Cow(resource, "punctuator")).Resolve(compiler)) {
      LOG(ERROR) << "failed to include section " << reference;
      return false;
    }
  }
  if (auto preset = resource->data->Traverse("recognizer/import_preset")) {
    if (!Is<ConfigValue>(preset))
      return false;
    auto preset_config_id = As<ConfigValue>(preset)->str();
    LOG(INFO) << "interpreting recognizer/import_preset: " << preset_config_id;
    Reference reference{preset_config_id, "recognizer", false};
    if (!IncludeReference{reference}
        .TargetedAt(Cow(resource, "recognizer")).Resolve(compiler)) {
      LOG(ERROR) << "failed to include section " << reference;
      return false;
    }
  }
  return true;
}

}  // namespace rime
