// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-02-18 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/expl/switcher_settings.h>

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

bool SwitcherSettings::Select(const Selection& selection) {
  selection_ = selection;
  ConfigListPtr schema_list(new ConfigList);
  BOOST_FOREACH(const std::string& schema_id, selection_) {
    ConfigMapPtr item(new ConfigMap);
    item->Set("schema", ConfigValuePtr(new ConfigValue(schema_id)));
    schema_list->Append(item);
  }
  return Customize("schema_list", schema_list);
}

bool SwitcherSettings::SetHotkeys(const std::string& hotkeys) {
  // TODO: not implemented; validation required
  return false;
}

void SwitcherSettings::GetAvailableSchemasFromDirectory(const fs::path& dir) {
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    EZLOGGERPRINT("Info: directory '%s' does not exist.", dir.string().c_str());
    return;
  }
  fs::directory_iterator it(dir);
  fs::directory_iterator end;
  for (; it != end; ++it) {
    std::string file_path(it->path().string());
    if (boost::ends_with(file_path, ".schema.yaml")) {
      Config config;
      if (config.LoadFromFile(file_path)) {
        SchemaInfo info;
        // required properties
        if (!config.GetString("schema/schema_id", &info.schema_id)) continue;
        if (!config.GetString("schema/name", &info.name)) continue;
        // check for duplicates
        bool duplicated = false;
        BOOST_FOREACH(const SchemaInfo& other, available_) {
          if (other.schema_id == info.schema_id) {
            duplicated = true;
            break;
          }
        }
        if (duplicated) continue;
        // details
        config.GetString("schema/version", &info.version);
        ConfigListPtr authors = config.GetList("schema/author");
        if (authors) {
          for (size_t i = 0; i < authors->size(); ++i) {
            ConfigValuePtr author = authors->GetValueAt(i);
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
  ConfigListPtr schema_list = config_.GetList("schema_list");
  if (!schema_list) {
    EZLOGGERPRINT("Warning: schema list not defined.");
    return;
  }
  ConfigList::Iterator it = schema_list->begin();
  for (; it != schema_list->end(); ++it) {
    ConfigMapPtr item = As<ConfigMap>(*it);
    if (!item) continue;
    ConfigValuePtr schema_property = item->GetValue("schema");
    if (!schema_property) continue;
    const std::string &schema_id(schema_property->str());
    selection_.push_back(schema_id);
  }
}

void SwitcherSettings::GetHotkeysFromConfig() {
  ConfigListPtr hotkeys = config_.GetList("switcher/hotkeys");
  if (!hotkeys) {
    EZLOGGERPRINT("Warning: hotkeys not defined.");
    return;
  }
  ConfigList::Iterator it = hotkeys->begin();
  for (; it != hotkeys->end(); ++it) {
    ConfigValuePtr item = As<ConfigValue>(*it);
    if (!item) continue;
    const std::string &hotkey(item->str());
    if (hotkey.empty()) continue;
    if (!hotkeys_.empty())
      hotkeys_ += ", ";
    hotkeys_ += hotkey;
  }
}

}  // namespace rime
