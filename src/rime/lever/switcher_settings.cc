//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-18 GONG Chen <chen.sst@gmail.com>
//
#include <utility>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/lever/switcher_settings.h>

namespace fs = std::filesystem;

namespace rime {

SwitcherSettings::SwitcherSettings(Deployer* deployer)
    : CustomSettings(deployer, "default", "Rime::SwitcherSettings") {}

bool SwitcherSettings::Load() {
  if (!CustomSettings::Load())
    return false;
  available_.clear();
  selection_.clear();
  hotkeys_.clear();
  GetAvailableSchemasFromDirectory(deployer_->shared_data_dir);
  GetAvailableSchemasFromDirectory(deployer_->user_data_dir);
  GetSelectedSchemasFromConfig();
  GetHotkeysFromConfig();
  return true;
}

bool SwitcherSettings::Select(Selection selection) {
  selection_ = std::move(selection);
  auto schema_list = New<ConfigList>();
  for (const string& schema_id : selection_) {
    auto item = New<ConfigMap>();
    item->Set("schema", New<ConfigValue>(schema_id));
    schema_list->Append(item);
  }
  return Customize("schema_list", schema_list);
}

bool SwitcherSettings::SetHotkeys(const string& hotkeys) {
  // TODO: not implemented; validation required
  return false;
}

void SwitcherSettings::GetAvailableSchemasFromDirectory(const path& dir) {
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    LOG(INFO) << "directory '" << dir << "' does not exist.";
    return;
  }
  for (fs::directory_iterator it(dir), end; it != end; ++it) {
    path file_path(it->path());
    if (boost::ends_with(file_path.u8string(), ".schema.yaml")) {
      Config config;
      if (config.LoadFromFile(file_path)) {
        SchemaInfo info;
        // required properties
        if (!config.GetString("schema/schema_id", &info.schema_id))
          continue;
        if (!config.GetString("schema/name", &info.name))
          continue;
        // check for duplicates
        bool duplicated = false;
        for (const SchemaInfo& other : available_) {
          if (other.schema_id == info.schema_id) {
            duplicated = true;
            break;
          }
        }
        if (duplicated)
          continue;
        // details
        config.GetString("schema/version", &info.version);
        if (auto authors = config.GetList("schema/author")) {
          for (size_t i = 0; i < authors->size(); ++i) {
            auto author = authors->GetValueAt(i);
            if (author && !author->str().empty()) {
              if (!info.author.empty())
                info.author += "\n";
              info.author += author->str();
            }
          }
        }
        config.GetString("schema/description", &info.description);
        // output path in native encoding.
        info.file_path = file_path.string();
        available_.push_back(info);
      }
    }
  }
}

void SwitcherSettings::GetSelectedSchemasFromConfig() {
  auto schema_list = config_.GetList("schema_list");
  if (!schema_list) {
    LOG(WARNING) << "schema list not defined.";
    return;
  }
  for (auto it = schema_list->begin(); it != schema_list->end(); ++it) {
    auto item = As<ConfigMap>(*it);
    if (!item)
      continue;
    auto schema_property = item->GetValue("schema");
    if (!schema_property)
      continue;
    const string& schema_id(schema_property->str());
    selection_.push_back(schema_id);
  }
}

void SwitcherSettings::GetHotkeysFromConfig() {
  auto hotkeys = config_.GetList("switcher/hotkeys");
  if (!hotkeys) {
    LOG(WARNING) << "hotkeys not defined.";
    return;
  }
  for (auto it = hotkeys->begin(); it != hotkeys->end(); ++it) {
    auto item = As<ConfigValue>(*it);
    if (!item)
      continue;
    const string& hotkey(item->str());
    if (hotkey.empty())
      continue;
    if (!hotkeys_.empty())
      hotkeys_ += ", ";
    hotkeys_ += hotkey;
  }
}

}  // namespace rime
