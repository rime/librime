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
#include <rime/expl/signature.h>
#include <rime/expl/switcher_settings.h>

namespace fs = boost::filesystem;

namespace rime {

SwitcherSettings::SwitcherSettings()
    : modified_(false) {
}

bool SwitcherSettings::Load(Deployer* deployer) {
  fs::path user_data_path(deployer->user_data_dir);
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path config_path(user_data_path / "default.yaml");
  Config config;
  if (!config.LoadFromFile(config_path.string())) {
    config_path = shared_data_path / "default.yaml";
    if (!config.LoadFromFile(config_path.string())) {
      EZLOGGERPRINT("Warning: cannot find 'default.yaml'.");
      return false;
    }
  }
  selection_.clear();
  GetSelectedSchemasFromConfig(&config);
  available_.clear();
  GetAvailableSchemasFromDirectory(shared_data_path);
  GetAvailableSchemasFromDirectory(user_data_path);
  hotkeys_.clear();
  GetHotkeysFromConfig(&config);
  modified_ = false;
  return true;
}

bool SwitcherSettings::Save(Deployer* deployer) {
  if (!modified_) return false;
  fs::path user_data_path(deployer->user_data_dir);
  fs::path shared_data_path(deployer->shared_data_dir);
  std::string custom_file((user_data_path / "default.custom.yaml").string());
  Config config;
  config.LoadFromFile(custom_file);
  ConfigListPtr schema_list(new ConfigList);
  BOOST_FOREACH(const std::string& schema_id, selection_) {
    ConfigMapPtr item(new ConfigMap);
    item->Set("schema", ConfigValuePtr(new ConfigValue(schema_id)));
    schema_list->Append(item);
  }
  ConfigMapPtr patch = config.GetMap("patch");
  if (!patch) {
    patch.reset(new ConfigMap);
  }
  patch->Set("schema_list", schema_list);
  // the branch 'patch' should be set as a whole, for
  // sometimes its sub-key may contain slashes which disables direct access.
  config.SetItem("patch", patch);
  Signature signature("Rime::SwitcherSettings");
  signature.Sign(&config, deployer);
  config.SaveToFile(custom_file);
  modified_ = false;
  return true;
}

bool SwitcherSettings::Select(const Selection& selection) {
  selection_ = selection;
  modified_ = true;
  return true;
}

bool SwitcherSettings::SetHotkeys(const std::string& hotkeys) {
  // TODO: not implemented; validation required
  return false;
}

void SwitcherSettings::GetSelectedSchemasFromConfig(Config* config) {
  ConfigListPtr schema_list = config->GetList("schema_list");
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

void SwitcherSettings::GetHotkeysFromConfig(Config* config) {
  ConfigListPtr hotkeys = config->GetList("switcher/hotkeys");
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
