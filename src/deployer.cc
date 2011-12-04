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

bool Deployer::InitializeInstallation() {
  EZLOGGERPRINT("initializing Rime installation.");
  fs::path shared_data_path(shared_data_dir);
  fs::path user_data_path(user_data_dir);
  fs::path installation_info(user_data_path / "installation.yaml");
  rime::Config config;
  bool existing_installation = false;
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
  if (!fs::exists(destination_path) || !fs::equivalent(source_path, destination_path)) {
    try {
      fs::copy_file(source_path, destination_path, fs::copy_option::overwrite_if_exists);
    }
    catch (...) {
      EZLOGGERPRINT("Error copying schema '%s' to Rime user directory.", schema_file.c_str());
      return false;
    }
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
  dict_compiler.Compile(dict_path.string(), schema_file);
  EZLOGGERPRINT("successfully compiled dictionary '%s'.", dict_name.c_str());
  return true;
}

}  // namespace rime
