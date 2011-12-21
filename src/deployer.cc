// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-01 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <rime_version.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>

namespace fs = boost::filesystem;

namespace rime {

static int CompareVersionString(const std::string &x, const std::string &y) {
  std::vector<std::string> xx, yy;
  boost::split(xx, x, boost::is_any_of("."));
  boost::split(yy, y, boost::is_any_of("."));
  size_t i = 0;
  for (; i < xx.size() && i < yy.size(); ++i) {
    int dx = atoi(xx[i].c_str());
    int dy = atoi(yy[i].c_str());
    if (dx != dy) return dx - dy;
    int c = xx[i].compare(yy[i]);
    if (c != 0) return c;
  }
  if (i < xx.size()) return 1;
  if (i < yy.size()) return -1;
  return 0;
}

static bool UpdateConfigFile(const fs::path &source_path,
                             const fs::path &dest_path,
                             const std::string &version_key) {
  if (fs::equivalent(source_path, dest_path)) {
    return true;
  }
  std::string source_version;
  std::string dest_version;
  rime::Config source_config;
  rime::Config dest_config;
  if (source_config.LoadFromFile(source_path.string())) {
    source_config.GetString(version_key, &source_version);
  }
  else {
    EZLOGGERPRINT("Error loading config from '%s'.",
                  source_path.string().c_str());
    return false;
  }
  if (dest_config.LoadFromFile(dest_path.string())) {
    dest_config.GetString(version_key, &dest_version);
  }
  if (!dest_version.empty() && isalpha(dest_version[0])) {
    std::string backup_file(dest_path.string() + ".bak");
    EZLOGGERPRINT("a customized config file is saved as '%s'.",
                  backup_file.c_str());
    fs::rename(dest_path, backup_file);
    dest_version.clear();
  }
  if (CompareVersionString(source_version, dest_version) <= 0) {
    EZLOGGERPRINT("config file '%s' is up-to-date.",
                  dest_path.string().c_str());
    return false;
  }
  EZLOGGERPRINT("updating config file '%s'.", dest_path.string().c_str());
  try {
    fs::copy_file(source_path, dest_path, fs::copy_option::overwrite_if_exists);
  }
  catch (...) {
    EZLOGGERPRINT("Error copying config file '%s' to user directory.",
                  source_path.string().c_str());
    return false;
  }
  return true;
}

bool Deployer::InitializeInstallation() {
  EZLOGGERPRINT("initializing Rime installation.");
  fs::path shared_data_path(shared_data_dir);
  fs::path user_data_path(user_data_dir);
  fs::path installation_info(user_data_path / "installation.yaml");
  rime::Config config;
  std::string installation_id;
  std::string last_distribution_code_name;
  std::string last_distribution_version;
  std::string last_rime_version;
  if (config.LoadFromFile(installation_info.string())) {
    if (config.GetString("installation_id", &installation_id)) {
      EZLOGGERPRINT("installation info exists. installation id: %s",
                    installation_id.c_str());
      // for now, treat installation_id as user_id
      user_id = installation_id;
    }
    if (config.GetString("distribution_code_name", &last_distribution_code_name)) {
      EZLOGGERPRINT("previous distribution: %s", last_distribution_code_name.c_str());
    }
    if (config.GetString("distribution_version", &last_distribution_version)) {
      EZLOGGERPRINT("previous distribution version: %s", last_distribution_version.c_str());
    }
    if (config.GetString("rime_version", &last_rime_version)) {
      EZLOGGERPRINT("previous Rime version: %s", last_rime_version.c_str());
    }
  }
  if (!installation_id.empty() &&
      last_distribution_code_name == distribution_code_name &&
      last_distribution_version == distribution_version &&
      last_rime_version == RIME_VERSION) {
    return true;
  }
  EZLOGGERPRINT("creating installation.");
  time_t now = time(NULL);
  std::string time_str(ctime(&now));
  boost::trim(time_str);
  if (installation_id.empty()) {
    installation_id = boost::uuids::to_string(boost::uuids::random_generator()());
    EZLOGGERPRINT("generated installation id: %s", installation_id.c_str());
    // for now, treat installation_id as user_id
    user_id = installation_id;
    config.SetString("installation_id", installation_id);
    config.SetString("install_time", time_str);
  }
  else {
    config.SetString("update_time", time_str);
  }
  if (!distribution_name.empty()) {
    config.SetString("distribution_name", distribution_name);
    EZLOGGERPRINT("distribution: %s", distribution_name.c_str());
  }
  if (!distribution_code_name.empty()) {
    config.SetString("distribution_code_name", distribution_code_name);
    EZLOGGERPRINT("distribution code name: %s", distribution_code_name.c_str());
  }
  if (!distribution_version.empty()) {
    config.SetString("distribution_version", distribution_version);
    EZLOGGERPRINT("distribution version: %s", distribution_version.c_str());
  }
  config.SetString("rime_version", RIME_VERSION);
  EZLOGGERPRINT("rime version: %s", RIME_VERSION);
  return config.SaveToFile(installation_info.string());
}

bool Deployer::InstallSchema(const std::string &schema_file) {
  fs::path source_path(schema_file);
  if (!fs::exists(source_path)) {
    EZLOGGERPRINT("Error installing schema: nonexistent file '%s'.", schema_file.c_str());
    return false;
  }
  Config config;
  std::string schema_id;
  if (!config.LoadFromFile(schema_file) ||
      !config.GetString("schema/schema_id", &schema_id) ||
      schema_id.empty()) {
    EZLOGGERPRINT("Error: invalid schema definition in '%s'.", schema_file.c_str());
    return false;
  }
  fs::path user_data_path(user_data_dir);
  fs::path destination_path(user_data_path / (schema_id + ".schema.yaml"));
  if (!UpdateConfigFile(source_path, destination_path, "schema/version")) {
    EZLOGGERPRINT("checking dictionary update for local copy of schema '%s'.", schema_id.c_str());
  }
  if (!config.LoadFromFile(destination_path.string())) {
    EZLOGGERPRINT("Error loading schema file '%s'.", destination_path.string().c_str());
    return false;
  }
  std::string dict_name;
  if (!config.GetString("translator/dictionary", &dict_name)) {
    // not requiring a dictionary
    return true;
  }
  fs::path dict_path(source_path.parent_path() / (dict_name + ".dict.yaml"));
  if (!fs::exists(dict_path)) {
    EZLOGGERPRINT("Error: source file for dictionary '%s' does not exist.", dict_name.c_str());
    return false;
  }
  DictionaryComponent component;
  scoped_ptr<Dictionary> dict(component.CreateDictionaryFromConfig(&config));
  if (!dict) {
    EZLOGGERPRINT("Error creating dictionary '%s'.", dict_name.c_str());
    return false;
  }
  EZLOGGERPRINT("preparing dictionary '%s'...", dict_name.c_str());
  DictCompiler dict_compiler(dict.get());
  dict_compiler.Compile(dict_path.string(), destination_path.string());
  EZLOGGERPRINT("dictionary '%s' is ready.", dict_name.c_str());
  return true;
}

bool Deployer::UpdateDistributedConfigFile(const std::string &file_name,
                                           const std::string &version_key) {
  fs::path shared_data_path(shared_data_dir);
  fs::path user_data_path(user_data_dir);
  fs::path shared_config_path(shared_data_path / file_name);
  fs::path config_path(user_data_path / file_name);
  if (fs::exists(shared_config_path)) {
    return UpdateConfigFile(shared_config_path, config_path, version_key);
  }
  else {
    EZLOGGERPRINT("Warning: '%s' is missing.", file_name.c_str());
    return false;
  }
}

bool Deployer::PrepareSchemas() {
  fs::path shared_data_path(shared_data_dir);
  fs::path user_data_path(user_data_dir);
  fs::path default_config_path(user_data_path / "default.yaml");
  EZLOGGERPRINT("preparing schemas.");
  UpdateDistributedConfigFile("default.yaml", "config_version");
  Config config;
  if (!config.LoadFromFile(default_config_path.string())) {
    EZLOGGERPRINT("Error loading default config from '%s'.",
                  default_config_path.string().c_str());
    return false;
  }
  // install schemas
  ConfigListPtr schema_list = config.GetList("schema_list");
  if (!schema_list) {
    EZLOGGERPRINT("Warning: schema list not defined.");
    return false;
  }
  size_t success = 0, failure = 0;
  ConfigList::Iterator it = schema_list->begin();
  for (; it != schema_list->end(); ++it) {
    ConfigMapPtr item = As<ConfigMap>(*it);
    if (!item) continue;
    ConfigValuePtr schema_property = item->GetValue("schema");
    if (!schema_property) continue;
    const std::string &schema_id(schema_property->str());
    fs::path schema_path = shared_data_path / (schema_id + ".schema.yaml");
    if (!fs::exists(schema_path)) {
      schema_path = user_data_path / (schema_id + ".schema.yaml");
    }      
    if (InstallSchema(schema_path.string()))
      ++success;
    else
      ++failure;
  }
  EZLOGGERPRINT("finished preparing schemas: %d success, %d failure.",
                success, failure);
  return true;
}

}  // namespace rime
