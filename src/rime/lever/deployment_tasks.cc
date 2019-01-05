//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-10 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <rime/common.h>
#include <rime/resource.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/setup.h>
#include <rime/ticket.h>
#include <rime/algo/utilities.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/lever/deployment_tasks.h>
#include <rime/lever/user_dict_manager.h>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::placeholders;

namespace fs = boost::filesystem;

namespace rime {

DetectModifications::DetectModifications(TaskInitializer arg) {
  try {
    data_dirs_ = boost::any_cast<vector<string>>(arg);
  }
  catch (const boost::bad_any_cast&) {
    LOG(ERROR) << "DetectModifications: invalid arguments.";
  }
}

bool DetectModifications::Run(Deployer* deployer) {
  time_t last_modified = 0;
  try {
    for (auto dir : data_dirs_) {
      fs::path p = fs::canonical(dir);
      last_modified = (std::max)(last_modified, fs::last_write_time(p));
      if (fs::is_directory(p)) {
        for (fs::directory_iterator iter(p), end; iter != end; ++iter) {
          fs::path entry(iter->path());
          if (fs::is_regular_file(fs::canonical(entry)) &&
              entry.extension().string() == ".yaml" &&
              entry.filename().string() != "user.yaml") {
            last_modified =
                (std::max)(last_modified, fs::last_write_time(entry));
          }
        }
      }
    }
  } catch(const fs::filesystem_error& ex) {
    LOG(ERROR) << "Error reading file information: " << ex.what();
    return true;
  }

  // TODO: store as 64-bit number to avoid the year 2038 problem
  int last_build_time = 0;
  {
    the<Config> user_config(Config::Require("user_config")->Create("user"));
    user_config->GetInt("var/last_build_time", &last_build_time);
  }
  if (last_modified > (time_t)last_build_time) {
    LOG(INFO) << "modifications detected. workspace needs update.";
    return true;
  }
  return false;
}

bool InstallationUpdate::Run(Deployer* deployer) {
  LOG(INFO) << "updating rime installation info.";
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path)) {
    LOG(INFO) << "creating user data dir: " << user_data_path.string();
    boost::system::error_code ec;
    if (!fs::create_directories(user_data_path, ec)) {
      LOG(ERROR) << "Error creating user data dir: " << user_data_path.string();
    }
  }
  fs::path installation_info(user_data_path / "installation.yaml");
  Config config;
  string installation_id;
  string last_distro_code_name;
  string last_distro_version;
  string last_rime_version;
  if (config.LoadFromFile(installation_info.string())) {
    if (config.GetString("installation_id", &installation_id)) {
      LOG(INFO) << "installation info exists. installation id: "
                << installation_id;
      // for now:
      deployer->user_id = installation_id;
    }
    if (!config.GetString("sync_dir", &deployer->sync_dir)) {
      deployer->sync_dir = (user_data_path / "sync").string();
    }
    LOG(INFO) << "sync dir: " << deployer->sync_dir;
    if (config.GetString("distribution_code_name", &last_distro_code_name)) {
      LOG(INFO) << "previous distribution: " << last_distro_code_name;
    }
    if (config.GetString("distribution_version", &last_distro_version)) {
      LOG(INFO) << "previous distribution version: " << last_distro_version;
    }
    if (config.GetString("rime_version", &last_rime_version)) {
      LOG(INFO) << "previous Rime version: " << last_rime_version;
    }
  }
  if (!installation_id.empty() &&
      last_distro_code_name == deployer->distribution_code_name &&
      last_distro_version == deployer->distribution_version &&
      last_rime_version == RIME_VERSION) {
    return true;
  }
  LOG(INFO) << "creating installation info.";
  time_t now = time(NULL);
  string time_str(ctime(&now));
  boost::trim(time_str);
  if (installation_id.empty()) {
    installation_id =
        boost::uuids::to_string(boost::uuids::random_generator()());
    LOG(INFO) << "generated installation id: " << installation_id;
    // for now:
    deployer->user_id = installation_id;
    config.SetString("installation_id", installation_id);
    config.SetString("install_time", time_str);
  }
  else {
    config.SetString("update_time", time_str);
  }
  if (!deployer->distribution_name.empty()) {
    config.SetString("distribution_name", deployer->distribution_name);
    LOG(INFO) << "distribution: " << deployer->distribution_name;
  }
  if (!deployer->distribution_code_name.empty()) {
    config.SetString("distribution_code_name",
                     deployer->distribution_code_name);
    LOG(INFO) << "distribution code name: "
              << deployer->distribution_code_name;
  }
  if (!deployer->distribution_version.empty()) {
    config.SetString("distribution_version",
                     deployer->distribution_version);
    LOG(INFO) << "distribution version: " << deployer->distribution_version;
  }
  config.SetString("rime_version", RIME_VERSION);
  LOG(INFO) << "Rime version: " << RIME_VERSION;
  return config.SaveToFile(installation_info.string());
}

bool WorkspaceUpdate::Run(Deployer* deployer) {
  LOG(INFO) << "updating workspace.";
  {
    the<DeploymentTask> t;
    t.reset(new ConfigFileUpdate("default.yaml", "config_version"));
    t->Run(deployer);
    // Deprecated: symbols.yaml is only used as source file
    //t.reset(new ConfigFileUpdate("symbols.yaml", "config_version"));
    //t->Run(deployer);
    t.reset(new SymlinkingPrebuiltDictionaries);
    t->Run(deployer);
  }

  the<Config> config(Config::Require("config")->Create("default"));
  if (!config) {
    LOG(ERROR) << "Error loading default config.";
    return false;
  }
  auto schema_list = config->GetList("schema_list");
  if (!schema_list) {
    LOG(WARNING) << "schema list not defined.";
    return false;
  }

  LOG(INFO) << "updating schemas.";
  int success = 0;
  int failure = 0;
  map<string, string> schemas;
  the<ResourceResolver> resolver(
      Service::instance().CreateResourceResolver(
          {"schema", "", ".schema.yaml"}));
  auto build_schema = [&](const string& schema_id) {
    if (schemas.find(schema_id) != schemas.end())  // already built
      return;
    LOG(INFO) << "schema: " << schema_id;
    string schema_path;
    if (schemas.find(schema_id) == schemas.end()) {
      schema_path = resolver->ResolvePath(schema_id).string();
      schemas[schema_id] = schema_path;
    }
    else {
      schema_path = schemas[schema_id];
    }
    if (schema_path.empty()) {
      LOG(WARNING) << "missing schema file for '" << schema_id << "'.";
      return;
    }
    the<DeploymentTask> t(new SchemaUpdate(schema_path));
    if (t->Run(deployer))
      ++success;
    else
      ++failure;
  };
  auto schema_component = Config::Require("schema");
  for (auto it = schema_list->begin(); it != schema_list->end(); ++it) {
    auto item = As<ConfigMap>(*it);
    if (!item)
      continue;
    auto schema_property = item->GetValue("schema");
    if (!schema_property)
      continue;
    const string& schema_id = schema_property->str();
    build_schema(schema_id);
    the<Config> schema_config(schema_component->Create(schema_id));
    if (!schema_config)
      continue;
    if (auto dependencies = schema_config->GetList("schema/dependencies")) {
      for (auto d = dependencies->begin(); d != dependencies->end(); ++d) {
        auto dependency = As<ConfigValue>(*d);
        if (!dependency)
          continue;
        const string& dependency_id = dependency->str();
        build_schema(dependency_id);
      }
    }
  }
  LOG(INFO) << "finished updating schemas: "
            << success << " success, " << failure << " failure.";

  the<Config> user_config(Config::Require("user_config")->Create("user"));
  // TODO: store as 64-bit number to avoid the year 2038 problem
  user_config->SetInt("var/last_build_time", (int)time(NULL));

  return failure == 0;
}

SchemaUpdate::SchemaUpdate(TaskInitializer arg) : verbose_(false) {
  try {
    schema_file_ = boost::any_cast<string>(arg);
  }
  catch (const boost::bad_any_cast&) {
    LOG(ERROR) << "SchemaUpdate: invalid arguments.";
  }
}

static bool MaybeCreateDirectory(fs::path dir) {
  if (!fs::exists(dir)) {
    boost::system::error_code ec;
    if (!fs::create_directories(dir, ec)) {
      LOG(ERROR) << "error creating directory '" << dir.string() << "'.";
      return false;
    }
  }
  return true;
}

static bool RemoveVersionSuffix(string* version, const string& suffix) {
    size_t suffix_pos = version->find(suffix);
    if (suffix_pos != string::npos) {
      version->erase(suffix_pos);
      return true;
    }
    return false;
}

static bool TrashDeprecatedUserCopy(const fs::path& shared_copy,
                                    const fs::path& user_copy,
                                    const string& version_key,
                                    const fs::path& trash) {
  if (!fs::exists(shared_copy) ||
      !fs::exists(user_copy) ||
      fs::equivalent(shared_copy, user_copy)) {
    return false;
  }
  string shared_copy_version;
  string user_copy_version;
  Config shared_config;
  if (shared_config.LoadFromFile(shared_copy.string())) {
    shared_config.GetString(version_key, &shared_copy_version);
    // treat "X.Y.minimal" as equal to (not greater than) "X.Y"
    // to avoid trashing the user installed full version
    RemoveVersionSuffix(&shared_copy_version, ".minimal");
  }
  Config user_config;
  bool is_customized_user_copy =
      user_config.LoadFromFile(user_copy.string()) &&
      user_config.GetString(version_key, &user_copy_version) &&
      RemoveVersionSuffix(&user_copy_version, ".custom.");
  int cmp = CompareVersionString(shared_copy_version, user_copy_version);
  // rime-installed user copy of the same version should be kept for integrity.
  // also it could have been manually edited by user.
  if (cmp > 0 || (cmp == 0 && is_customized_user_copy)) {
    if (!MaybeCreateDirectory(trash)) {
      return false;
    }
    fs::path backup = trash / user_copy.filename();
    boost::system::error_code ec;
    fs::rename(user_copy, backup, ec);
    if (ec) {
      LOG(ERROR) << "error trashing file " << user_copy.string();
      return false;
    }
    return true;
  }
  return false;
}

bool SchemaUpdate::Run(Deployer* deployer) {
  fs::path source_path(schema_file_);
  if (!fs::exists(source_path)) {
    LOG(ERROR) << "Error updating schema: nonexistent file '"
               << schema_file_ << "'.";
    return false;
  }
  string schema_id;
  the<Config> config(new Config);
  if (!config->LoadFromFile(schema_file_) ||
      !config->GetString("schema/schema_id", &schema_id) ||
      schema_id.empty()) {
    LOG(ERROR) << "invalid schema definition in '" << schema_file_ << "'.";
    return false;
  }

  the<DeploymentTask> config_file_update(
      new ConfigFileUpdate(schema_id + ".schema.yaml", "schema/version"));
  if (!config_file_update->Run(deployer)) {
    return false;
  }
  // reload compiled config
  config.reset(Config::Require("schema")->Create(schema_id));
  string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    // not requiring a dictionary
    return true;
  }
  Schema schema(schema_id, config.release());
  the<Dictionary> dict(
      Dictionary::Require("dictionary")->Create({&schema, "translator"}));
  if (!dict) {
    LOG(ERROR) << "Error creating dictionary '" << dict_name << "'.";
    return false;
  }

  LOG(INFO) << "preparing dictionary '" << dict_name << "'.";
  fs::path user_data_path(deployer->user_data_dir);
  if (!MaybeCreateDirectory(user_data_path / "build")) {
    return false;
  }
  DictCompiler dict_compiler(dict.get(), "build/");
  if (verbose_) {
    dict_compiler.set_options(DictCompiler::kRebuild | DictCompiler::kDump);
  }
  the<ResourceResolver> resolver(
      Service::instance().CreateResourceResolver(
          {"compiled_schema", "build/", ".schema.yaml"}));
  resolver->set_root_path(user_data_path);
  auto compiled_schema = resolver->ResolvePath(schema_id).string();
  if (!dict_compiler.Compile(compiled_schema)) {
    LOG(ERROR) << "dictionary '" << dict_name << "' failed to compile.";
    return false;
  }
  LOG(INFO) << "dictionary '" << dict_name << "' is ready.";
  return true;
}

ConfigFileUpdate::ConfigFileUpdate(TaskInitializer arg) {
  try {
    auto p = boost::any_cast<pair<string, string>>(arg);
    file_name_ = p.first;
    version_key_ = p.second;
  }
  catch (const boost::bad_any_cast&) {
    LOG(ERROR) << "ConfigFileUpdate: invalid arguments.";
  }
}

static bool ConfigNeedsUpdate(Config* config) {
  auto build_info = (*config)["__build_info"];
  if (!build_info.IsMap()) {
    LOG(INFO) << "missing build info";
    return true;
  }
  auto timestamps = build_info["timestamps"];
  if (!timestamps.IsMap()) {
    LOG(INFO) << "missing timestamps";
    return true;
  }
  the<ResourceResolver> resolver(
      Service::instance().CreateResourceResolver(
          {"config_source_file", "", ".yaml"}));
  for (auto entry : *timestamps.AsMap()) {
    auto value = As<ConfigValue>(entry.second);
    int recorded_time = 0;
    if (!value || !value->GetInt(&recorded_time)) {
      LOG(WARNING) << "invalid timestamp for " << entry.first;
      return true;
    }
    fs::path source_file = resolver->ResolvePath(entry.first);
    if (!fs::exists(source_file)) {
      if (recorded_time) {
        LOG(INFO) << "source file no longer exists: " << source_file.string();
        return true;
      }
      continue;
    }
    if (recorded_time != (int) fs::last_write_time(source_file)) {
      LOG(INFO) << "source file " << (recorded_time ? "changed: " : "added: ")
                << source_file.string();
      return true;
    }
  }
  return false;
}

bool ConfigFileUpdate::Run(Deployer* deployer) {
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path user_data_path(deployer->user_data_dir);
  // trash depecated user copy created by an older version of Rime
  fs::path source_config_path(shared_data_path / file_name_);
  fs::path dest_config_path(user_data_path / file_name_);
  fs::path trash = user_data_path / "trash";
  if (TrashDeprecatedUserCopy(source_config_path,
                              dest_config_path,
                              version_key_,
                              trash)) {
    LOG(INFO) << "deprecated user copy of '" << file_name_
              << "' is moved to " << trash;
  }
  // build the config file if needs update
  the<Config> config(Config::Require("config")->Create(file_name_));
  if (ConfigNeedsUpdate(config.get())) {
    if (!MaybeCreateDirectory(user_data_path / "build")) {
      return false;
    }
    config.reset(Config::Require("config_builder")->Create(file_name_));
  }
  return true;
}

bool PrebuildAllSchemas::Run(Deployer* deployer) {
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path user_data_path(deployer->user_data_dir);
  if (!fs::exists(shared_data_path) || !fs::is_directory(shared_data_path))
    return false;
  bool success = true;
  for (fs::directory_iterator iter(shared_data_path), end;
       iter != end; ++iter) {
    fs::path entry(iter->path());
    if (boost::ends_with(entry.string(), ".schema.yaml")) {
      the<DeploymentTask> t(new SchemaUpdate(entry.string()));
      if (!t->Run(deployer))
        success = false;
    }
  }
  return success;
}

bool SymlinkingPrebuiltDictionaries::Run(Deployer* deployer) {
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path user_data_path(deployer->user_data_dir);
  if (!fs::exists(shared_data_path) || !fs::is_directory(shared_data_path) ||
      !fs::exists(user_data_path) || !fs::is_directory(user_data_path) ||
      fs::equivalent(shared_data_path, user_data_path))
    return false;
  bool success = false;
  // remove symlinks to shared data files created by previous version
  for (fs::directory_iterator test(user_data_path), end;
       test != end; ++test) {
    fs::path entry(test->path());
    if (fs::is_symlink(entry)) {
      try {
        // a symlink becomes dangling if the target file is no longer provided
        boost::system::error_code ec;
        auto target_path = fs::canonical(entry, ec);
        bool bad_link = bool(ec);
        bool linked_to_shared_data =
            !bad_link &&
            target_path.has_parent_path() &&
            fs::equivalent(shared_data_path, target_path.parent_path());
        if (bad_link || linked_to_shared_data) {
          LOG(INFO) << "removing symlink: " << entry.filename().string();
          fs::remove(entry);
        }
      }
      catch (const fs::filesystem_error& ex) {
        LOG(ERROR) << entry << ": " << ex.what();
        success = false;
      }
    }
  }
  return success;
}

bool UserDictUpgrade::Run(Deployer* deployer) {
  LoadModules(kLegacyModules);
  auto legacy_userdb_component = UserDb::Require("legacy_userdb");
  if (!legacy_userdb_component) {
    return true;  // nothing to upgrade
  }
  UserDictManager manager(deployer);
  UserDictList dicts;
  manager.GetUserDictList(&dicts, legacy_userdb_component);
  bool ok = true;
  for (auto it = dicts.cbegin(); it != dicts.cend(); ++it) {
    if (!manager.UpgradeUserDict(*it))
      ok = false;
  }
  return ok;
}

bool UserDictSync::Run(Deployer* deployer) {
  UserDictManager mgr(deployer);
  return mgr.SynchronizeAll();
}

static bool IsCustomizedCopy(const string& file_name) {
  if (boost::ends_with(file_name, ".yaml") &&
      !boost::ends_with(file_name, ".custom.yaml")) {
    Config config;
    string checksum;
    if (config.LoadFromFile(file_name) &&
        config.GetString("customization", &checksum)) {
      return true;
    }
  }
  return false;
}

bool BackupConfigFiles::Run(Deployer* deployer) {
  LOG(INFO) << "backing up config files.";
  fs::path user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path))
    return false;
  fs::path backup_dir(deployer->user_data_sync_dir());
  if (!MaybeCreateDirectory(backup_dir)) {
    return false;
  }
  int success = 0, failure = 0, latest = 0, skipped = 0;
  for (fs::directory_iterator iter(user_data_path), end;
       iter != end; ++iter) {
    fs::path entry(iter->path());
    if (!fs::is_regular_file(entry))
      continue;
    auto file_extension = entry.extension().string();
    bool is_yaml_file = file_extension == ".yaml";
    bool is_text_file = file_extension == ".txt";
    if (!is_yaml_file && !is_text_file)
      continue;
    fs::path backup = backup_dir / entry.filename();
    if (fs::exists(backup) &&
        Checksum(backup.string()) == Checksum(entry.string())) {
      ++latest;  // already up-to-date
      continue;
    }
    if (is_yaml_file && IsCustomizedCopy(entry.string())) {
      ++skipped;  // customized copy
      continue;
    }
    boost::system::error_code ec;
    fs::copy_file(entry, backup, fs::copy_option::overwrite_if_exists, ec);
    if (ec) {
      LOG(ERROR) << "error backing up file " << backup.string();
      ++failure;
    }
    else {
      ++success;
    }
  }
  LOG(INFO) << "backed up " << success << " config files to "
            << backup_dir.string() << ", " << failure << " failed, "
            << latest << " up-to-date, "
            << skipped << " skipped.";
  return !failure;
}

bool CleanupTrash::Run(Deployer* deployer) {
  LOG(INFO) << "clean up trash.";
  fs::path user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path))
    return false;
  fs::path trash = user_data_path / "trash";
  int success = 0, failure = 0;
  for (fs::directory_iterator iter(user_data_path), end;
       iter != end; ++iter) {
    fs::path entry(iter->path());
    if (!fs::is_regular_file(entry))
      continue;
    auto filename = entry.filename().string();
    if (filename == "rime.log" ||
        boost::ends_with(filename, ".bin") ||
        boost::ends_with(filename, ".reverse.kct") ||
        boost::ends_with(filename, ".userdb.kct.old") ||
        boost::ends_with(filename, ".userdb.kct.snapshot")) {
      if (!success && !MaybeCreateDirectory(trash)) {
        return false;
      }
      fs::path backup = trash / entry.filename();
      boost::system::error_code ec;
      fs::rename(entry, backup, ec);
      if (ec) {
        LOG(ERROR) << "error clean up file " << entry.string();
        ++failure;
      }
      else {
        ++success;
      }
    }
  }
  if (success) {
    LOG(INFO) << "moved " << success << " files to " << trash.string();
  }
  return !failure;
}

bool CleanOldLogFiles::Run(Deployer* deployer) {
  char ymd[12] = {0};
  time_t now = time(NULL);
  strftime(ymd, sizeof(ymd), ".%Y%m%d", localtime(&now));
  string today(ymd);
  DLOG(INFO) << "today: " << today;

  vector<string> dirs;
#ifdef RIME_ENABLE_LOGGING
#ifdef _WIN32
  // work-around: google::GetExistingTempDirectories crashes on windows 7
  char tmp[MAX_PATH];
  if (GetTempPathA(MAX_PATH, tmp))
    dirs.push_back(tmp);
#else
  google::GetExistingTempDirectories(&dirs);
#endif  // _WIN32
#endif  // RIME_ENABLE_LOGGING
  DLOG(INFO) << "scanning " << dirs.size() << " temp directory for log files.";

  bool success = true;
  int removed = 0;
  for (auto i = dirs.cbegin(); i != dirs.cend(); ++i) {
    DLOG(INFO) << "temp directory: " << *i;
    for (fs::directory_iterator j(*i), end; j != end; ++j) {
      fs::path entry(j->path());
      string file_name(entry.filename().string());
      try {
        if (fs::is_regular_file(entry) &&
            !fs::is_symlink(entry) &&
            boost::starts_with(file_name, "rime.") &&
            !boost::contains(file_name, today)) {
          DLOG(INFO) << "removing log file '" << file_name << "'.";
          fs::remove(entry);
          ++removed;
        }
      }
      catch (const fs::filesystem_error& ex) {
        LOG(ERROR) << ex.what();
        success = false;
      }
    }
  }
  if (removed != 0) {
    LOG(INFO) << "cleaned " << removed << " log files.";
  }
  return success;
}

}  // namespace rime
