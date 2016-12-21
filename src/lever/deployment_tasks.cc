//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-10 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/setup.h>
#include <rime/ticket.h>
#include <rime/algo/utilities.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/lever/customizer.h>
#include <rime/lever/deployment_tasks.h>
#include <rime/lever/user_dict_manager.h>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::placeholders;

namespace fs = boost::filesystem;

namespace rime {

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
    // since brise 0.18
    t.reset(new ConfigFileUpdate("symbols.yaml", "config_version"));
    t->Run(deployer);
    t.reset(new SymlinkingPrebuiltDictionaries);
    t->Run(deployer);
  }

  fs::path user_data_path(deployer->user_data_dir);
  fs::path default_config_path(user_data_path / "default.yaml");
  Config config;
  if (!config.LoadFromFile(default_config_path.string())) {
    LOG(ERROR) << "Error loading default config from '"
               << default_config_path.string() << "'.";
    return false;
  }
  auto schema_list = config.GetList("schema_list");
  if (!schema_list) {
    LOG(WARNING) << "schema list not defined.";
    return false;
  }

  LOG(INFO) << "updating schemas.";
  int success = 0;
  int failure = 0;
  map<string, string> schemas;
  for (auto it = schema_list->begin(); it != schema_list->end(); ++it) {
    auto item = As<ConfigMap>(*it);
    if (!item)
      continue;
    auto schema_property = item->GetValue("schema");
    if (!schema_property)
      continue;
    const string& schema_id(schema_property->str());
    LOG(INFO) << "schema: " << schema_id;
    string schema_path;
    if (schemas.find(schema_id) == schemas.end()) {
      schema_path = GetSchemaPath(deployer, schema_id, true);
      schemas[schema_id] = schema_path;
    }
    else {
      schema_path = schemas[schema_id];
    }
    if (schema_path.empty()) {
      LOG(WARNING) << "missing schema file for '" << schema_id << "'.";
      continue;
    }
    // build schema
    the<DeploymentTask> t(new SchemaUpdate(schema_path));
    if (t->Run(deployer))
      ++success;
    else
      ++failure;
  }
  // find dependencies
  for (auto s = schemas.cbegin(); s != schemas.cend(); ++s) {
    Config schema_config;
    // user could have customized dependencies in the resulting schema
    string user_schema_path = GetSchemaPath(deployer, s->first, false);
    if (!schema_config.LoadFromFile(user_schema_path))
      continue;
    auto dependencies = schema_config.GetList("schema/dependencies");
    if (!dependencies)
      continue;
    for (auto d = dependencies->begin(); d != dependencies->end(); ++d) {
      auto dependency = As<ConfigValue>(*d);
      if (!dependency)
        continue;
      string dependency_id(dependency->str());
      if (schemas.find(dependency_id) != schemas.end())  // already built
        continue;
      LOG(INFO) << "new dependency: " << dependency_id;
      string dependency_path = GetSchemaPath(deployer, dependency_id, true);
      schemas[dependency_id] = dependency_path;
      if (dependency_path.empty()) {
        LOG(WARNING) << "missing schema file for dependency '" << dependency_id << "'.";
        continue;
      }
      // build dependency
      the<DeploymentTask> t(new SchemaUpdate(dependency_path));
      if (t->Run(deployer))
        ++success;
      else
        ++failure;
    }
  }
  LOG(INFO) << "finished updating schemas: "
            << success << " success, " << failure << " failure.";
  return failure == 0;
}

string WorkspaceUpdate::GetSchemaPath(Deployer* deployer,
                                           const string& schema_id,
                                           bool prefer_shared_copy) {
  fs::path schema_path;
  if (prefer_shared_copy) {
    fs::path shared_data_path(deployer->shared_data_dir);
    schema_path = shared_data_path / (schema_id + ".schema.yaml");
    if (!fs::exists(schema_path))
      schema_path.clear();
  }
  if (schema_path.empty()) {
    fs::path user_data_path(deployer->user_data_dir);
    schema_path = user_data_path / (schema_id + ".schema.yaml");
    if (!fs::exists(schema_path))
      schema_path.clear();
  }
  return schema_path.string();
}

SchemaUpdate::SchemaUpdate(TaskInitializer arg) : verbose_(false) {
  try {
    schema_file_ = boost::any_cast<string>(arg);
  }
  catch (const boost::bad_any_cast&) {
    LOG(ERROR) << "SchemaUpdate: invalid arguments.";
  }
}

static string find_dict_file(const string& dict_file_name,
                                  const fs::path& shared_data_path,
                                  const fs::path& user_data_path) {
  fs::path dict_path(user_data_path / dict_file_name);
  if (!fs::exists(dict_path)) {
    dict_path = shared_data_path / dict_file_name;
    if (!fs::exists(dict_path)) {
      LOG(ERROR) << "source file '" << dict_file_name << "' does not exist.";
      return string();
    }
  }
  return dict_path.string();
}

bool SchemaUpdate::Run(Deployer* deployer) {
  fs::path source_path(schema_file_);
  if (!fs::exists(source_path)) {
    LOG(ERROR) << "Error updating schema: nonexistent file '"
               << schema_file_ << "'.";
    return false;
  }
  string schema_id;
  {
    Config source;
    if (!source.LoadFromFile(schema_file_) ||
        !source.GetString("schema/schema_id", &schema_id) ||
        schema_id.empty()) {
      LOG(ERROR) << "invalid schema definition in '" << schema_file_ << "'.";
      return false;
    }
  }
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path user_data_path(deployer->user_data_dir);
  fs::path destination_path(user_data_path / (schema_id + ".schema.yaml"));
  Customizer customizer(source_path, destination_path, "schema/version");
  if (customizer.UpdateConfigFile()) {
    LOG(INFO) << "schema '" << schema_id << "' is updated.";
  }

  Schema schema(schema_id, new Config);
  Config* config = schema.config();
  if (!config || !config->LoadFromFile(destination_path.string())) {
    LOG(ERROR) << "Error loading schema file '"
               << destination_path.string() << "'.";
    return false;
  }
  string dict_name;
  if (!config->GetString("translator/dictionary", &dict_name)) {
    // not requiring a dictionary
    return true;
  }
  DictionaryComponent component;
  the<Dictionary> dict(component.Create({&schema, "translator"}));
  if (!dict) {
    LOG(ERROR) << "Error creating dictionary '" << dict_name << "'.";
    return false;
  }
  LOG(INFO) << "preparing dictionary '" << dict_name << "'.";
  DictFileFinder finder = std::bind(&find_dict_file,
                                    _1, shared_data_path, user_data_path);
  DictCompiler dict_compiler(dict.get(), finder);
  if (verbose_) {
    dict_compiler.set_options(DictCompiler::kRebuild | DictCompiler::kDump);
  }
  if (!dict_compiler.Compile(destination_path.string())) {
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

bool ConfigFileUpdate::Run(Deployer* deployer) {
  fs::path shared_data_path(deployer->shared_data_dir);
  fs::path user_data_path(deployer->user_data_dir);
  fs::path source_config_path(shared_data_path / file_name_);
  fs::path dest_config_path(user_data_path / file_name_);
  if (!fs::exists(source_config_path)) {
    LOG(WARNING) << "'" << file_name_
                 << "' is missing from shared data directory.";
    source_config_path = dest_config_path;
  }
  Customizer customizer(source_config_path, dest_config_path, version_key_);
  return customizer.UpdateConfigFile();
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
  // test existing link
  for (fs::directory_iterator test(user_data_path), end;
       test != end; ++test) {
    fs::path entry(test->path());
    if (fs::is_symlink(entry) && entry.extension().string() == ".bin") {
      try {
        if (!fs::exists(entry)) {
          LOG(INFO) << "removing dangling symlink: "
                    << entry.filename().string();
          fs::remove(entry);
        }
      }
      catch (const fs::filesystem_error& ex) {
        LOG(ERROR) << ex.what();
        success = false;
      }
    }
  }
  // create link
  for (fs::directory_iterator iter(shared_data_path), end;
       iter != end; ++iter) {
    fs::path entry(iter->path());
    fs::path link(user_data_path / entry.filename());
    try {
      if (fs::is_regular_file(entry) &&
          entry.extension().string() == ".bin" &&
          !fs::exists(link)) {
        LOG(INFO) << "symlinking '" << entry.filename().string() << "'.";
        fs::create_symlink(entry, link);
      }
    }
    catch (const fs::filesystem_error& ex) {
      LOG(ERROR) << ex.what();
      success = false;
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

bool BackupConfigFiles::Run(Deployer* deployer) {
  LOG(INFO) << "backing up config files.";
  fs::path user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path))
    return false;
  fs::path backup_dir(deployer->user_data_sync_dir());
  if (!fs::exists(backup_dir)) {
    boost::system::error_code ec;
    if (!fs::create_directories(backup_dir, ec)) {
      LOG(ERROR) << "error creating directory '"
                 << backup_dir.string() << "'.";
      return false;
    }
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
    if (is_yaml_file && !boost::ends_with(entry.string(), ".custom.yaml")) {
      Config config;
      string checksum;
      if (config.LoadFromFile(entry.string()) &&
          config.GetString("customization", &checksum)) {
        ++skipped;  // customized copy
        continue;
      }
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
        boost::ends_with(filename, ".reverse.kct") ||
        boost::ends_with(filename, ".userdb.kct.old") ||
        boost::ends_with(filename, ".userdb.kct.snapshot")) {
      if (!success && !failure && !fs::exists(trash)) {
        boost::system::error_code ec;
        if (!fs::create_directories(trash, ec)) {
          LOG(ERROR) << "error creating directory '" << trash.string() << "'.";
          return false;
        }
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
