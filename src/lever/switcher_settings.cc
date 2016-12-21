//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-18 GONG Chen <chen.sst@gmail.com>
//
#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/lever/switcher_settings.h>

namespace fs = boost::filesystem;

namespace rime {

SwitcherSettings::SwitcherSettings(Deployer* deployer)
    : CustomSettings(deployer, "default", "Rime::SwitcherSettings") {
}

bool SwitcherSettings::Load() {
  if (!CustomSettings::Load())
    return false;
  fs::path user_data_path(deployer_->user_data_dir);
  fs::path shared_data_path(deployer_->shared_data_dir);
  available_.clear();
  selection_.clear();
  hotkeys_.clear();
  GetAvailableSchemasFromDirectory(shared_data_path);
  GetAvailableSchemasFromDirectory(user_data_path);
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

void SwitcherSettings::GetAvailableSchemasFromDirectory(const fs::path& dir) {
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    LOG(INFO) << "directory '" << dir.string() << "' does not exist.";
    return;
  }
  for (fs::directory_iterator it(dir), end;
       it != end; ++it) {
    string file_path(it->path().string());
    if (boost::ends_with(file_path, ".schema.yaml")) {
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
        info.file_path = file_path;
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
