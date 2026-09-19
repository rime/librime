//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-10 GONG Chen <chen.sst@gmail.com>
//

#include <rime/build_config.h>

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <future>
#include <thread>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <rime/common.h>
#include <rime/resource.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/setup.h>
#include <rime/ticket.h>
#include <rime/algo/fs.h>
#include <rime/algo/utilities.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/lever/deployment_tasks.h>
#include <rime/lever/user_dict_manager.h>
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <unistd.h>
#elif !defined(_WIN32)
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace rime {

DetectModifications::DetectModifications(TaskInitializer arg) {
  try {
    data_dirs_ = std::any_cast<vector<path>>(arg);
  } catch (const std::bad_any_cast&) {
    LOG(ERROR) << "DetectModifications: invalid arguments.";
  }
}

bool DetectModifications::Run(Deployer* deployer) {
  time_t last_modified = 0;
  try {
    for (auto dir : data_dirs_) {
      path p = fs::canonical(dir);
      last_modified = (std::max)(last_modified,
                                 filesystem::to_time_t(fs::last_write_time(p)));
      if (fs::is_directory(p)) {
        for (fs::directory_iterator iter(p), end; iter != end; ++iter) {
          path entry(iter->path());
          if (fs::is_regular_file(fs::canonical(entry)) &&
              entry.extension().u8string() == ".yaml" &&
              entry.filename().u8string() != "user.yaml") {
            last_modified =
                (std::max)(last_modified,
                           filesystem::to_time_t(fs::last_write_time(entry)));
          }
        }
      }
    }
  } catch (const fs::filesystem_error& ex) {
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
  const path& shared_data_path(deployer->shared_data_dir);
  const path& user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path)) {
    LOG(INFO) << "creating user data dir: " << user_data_path;
    std::error_code ec;
    if (!fs::create_directories(user_data_path, ec)) {
      LOG(ERROR) << "Error creating user data dir: " << user_data_path;
    }
  }
  path installation_info(user_data_path / "installation.yaml");
  Config config;
  string installation_id;
  string last_distro_code_name;
  string last_distro_version;
  string last_rime_version;
  if (config.LoadFromFile(installation_info)) {
    if (config.GetString("installation_id", &installation_id)) {
      LOG(INFO) << "installation info exists. installation id: "
                << installation_id;
      // for now:
      deployer->user_id = installation_id;
    }
    string sync_dir;
    if (config.GetString("sync_dir", &sync_dir)) {
      deployer->sync_dir = path(sync_dir);
    } else {
      deployer->sync_dir = user_data_path / "sync";
    }
    LOG(INFO) << "sync dir: " << deployer->sync_dir;
    bool backup_config_files;
    if (config.GetBool("backup_config_files", &backup_config_files)) {
      deployer->backup_config_files = backup_config_files;
    }
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
  } else {
    config.SetString("update_time", time_str);
  }
  if (!deployer->distribution_name.empty()) {
    config.SetString("distribution_name", deployer->distribution_name);
    LOG(INFO) << "distribution: " << deployer->distribution_name;
  }
  if (!deployer->distribution_code_name.empty()) {
    config.SetString("distribution_code_name",
                     deployer->distribution_code_name);
    LOG(INFO) << "distribution code name: " << deployer->distribution_code_name;
  }
  if (!deployer->distribution_version.empty()) {
    config.SetString("distribution_version", deployer->distribution_version);
    LOG(INFO) << "distribution version: " << deployer->distribution_version;
  }
  config.SetString("rime_version", RIME_VERSION);
  LOG(INFO) << "Rime version: " << RIME_VERSION;
  return config.SaveToFile(installation_info);
}

static uint64_t DetectTotalPhysicalMemory() {
#ifdef _WIN32
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status)) {
    return status.ullTotalPhys;
  }
#elif defined(__APPLE__)
  int mib[2] = {CTL_HW, HW_MEMSIZE};
  uint64_t memsize = 0;
  size_t len = sizeof(memsize);
  if (sysctl(mib, 2, &memsize, &len, nullptr, 0) == 0) {
    return memsize;
  }
#else
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages > 0 && page_size > 0) {
    return (uint64_t)pages * (uint64_t)page_size;
  }
#endif
  return 0;
}

static bool MaybeCreateDirectory(path dir) {
  std::error_code ec;
  if (fs::create_directories(dir, ec)) {
    return true;
  }

  if (fs::exists(dir)) {
    return true;
  }
  LOG(ERROR) << "error creating directory '" << dir << "'.";
  return false;
}

// a dictionary to be compiled for a schema; multiple schemas may share one
struct SchemaCompileUnit {
  string schema_id;
  string dict_name;
  path compiled_schema_path;
  vector<string> artifact_stems;  // output file names (without extension)
  an<Dictionary> dict;
};

static bool CompileDictionary(Dictionary* dict,
                              const path& compiled_schema_path,
                              bool verbose = false) {
  DictCompiler dict_compiler(dict);
  if (verbose) {
    dict_compiler.set_options(DictCompiler::kRebuild | DictCompiler::kDump);
  }
  return dict_compiler.Compile(compiled_schema_path);
}

bool WorkspaceUpdate::Run(Deployer* deployer) {
  LOG(INFO) << "updating workspace.";
  {
    the<DeploymentTask> t;
    t.reset(new ConfigFileUpdate("default.yaml", "config_version"));
    t->Run(deployer);
    // Deprecated: symbols.yaml is only used as source file
    // t.reset(new ConfigFileUpdate("symbols.yaml", "config_version"));
    // t->Run(deployer);
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

  the<ResourceResolver> resolver(Service::instance().CreateResourceResolver(
      {"schema_source_file", "", ".schema.yaml"}));
  auto schema_component = Config::Require("schema");

  // Phase 1 (serial): update schema config files, resolve the dictionaries to
  // compile, and collect them into compile units. All accesses to the shared,
  // non-thread-safe component caches happen in this phase.
  vector<SchemaCompileUnit> units;
  map<string, path> schemas;
  std::function<void(const string&, bool)> process_schema =
      [&](const string& schema_id, bool as_dependency) {
        if (schemas.find(schema_id) != schemas.end())  // already processed
          return;
        LOG(INFO) << "schema: " << schema_id;
        path schema_path = resolver->ResolvePath(schema_id);
        schemas[schema_id] = schema_path;
        if (schema_path.empty() || !fs::exists(schema_path)) {
          if (as_dependency) {
            LOG(WARNING)
                << "missing input schema; skipped unsatisfied dependency: "
                << schema_id;
          } else {
            LOG(ERROR) << "missing input schema: " << schema_id;
            ++failure;
          }
          return;
        }
        the<DeploymentTask> config_file_update(
            new ConfigFileUpdate(schema_id + ".schema.yaml", "schema/version"));
        if (!config_file_update->Run(deployer)) {
          ++failure;
          return;
        }
        the<Config> schema_config(schema_component->Create(schema_id));
        if (!schema_config) {
          ++failure;
          return;
        }
        string dict_name;
        if (!schema_config->GetString("translator/dictionary", &dict_name)) {
          ++success;  // not requiring a dictionary
          return;
        }
        Schema schema(schema_id, schema_config.release());
        the<Dictionary> dict(
            Dictionary::Require("dictionary")->Create({&schema, "translator"}));
        if (!dict) {
          LOG(ERROR) << "Error creating dictionary '" << dict_name << "'.";
          ++failure;
          return;
        }
        SchemaCompileUnit unit;
        unit.schema_id = schema_id;
        unit.dict_name = dict_name;
        the<ResourceResolver> compiled_resolver(
            Service::instance().CreateDeployedResourceResolver(
                {"compiled_schema", "", ".schema.yaml"}));
        unit.compiled_schema_path = compiled_resolver->ResolvePath(schema_id);
        // output files this unit may write; files shared by two units must not
        // be written concurrently
        for (const auto& table : dict->tables()) {
          unit.artifact_stems.push_back(table->file_path().stem().u8string());
        }
        unit.artifact_stems.push_back(
            dict->prism()->file_path().stem().u8string());
        unit.artifact_stems.push_back(dict->name());  // reverse db
        unit.dict = std::move(dict);
        units.push_back(std::move(unit));
      };
  for (auto it = schema_list->begin(); it != schema_list->end(); ++it) {
    auto item = As<ConfigMap>(*it);
    if (!item)
      continue;
    auto schema_property = item->GetValue("schema");
    if (!schema_property)
      continue;
    process_schema(schema_property->str(), false);
    // compile the schema's own dictionary before its direct dependencies,
    // matching the upstream serial build order (only one level; dependencies
    // of dependencies are not compiled)
    the<Config> schema_config(schema_component->Create(schema_property->str()));
    if (!schema_config)
      continue;
    if (auto dependencies = schema_config->GetList("schema/dependencies")) {
      for (auto d = dependencies->begin(); d != dependencies->end(); ++d) {
        auto dependency = As<ConfigValue>(*d);
        if (!dependency)
          continue;
        process_schema(dependency->str(), true);
      }
    }
  }

  // Phase 2: compile the dictionaries. Units that share output files (the same
  // dictionary, or a dictionary that is another schema's pack) are compiled
  // serially; independent units may be compiled in parallel.
  if (!MaybeCreateDirectory(deployer->staging_dir)) {
    return false;
  }
  map<string, vector<size_t>> artifact_owners;
  for (size_t i = 0; i < units.size(); ++i) {
    for (const auto& artifact : units[i].artifact_stems) {
      artifact_owners[artifact].push_back(i);
    }
  }
  vector<bool> exclusive(units.size(), true);
  for (const auto& entry : artifact_owners) {
    if (entry.second.size() > 1) {
      for (size_t i : entry.second) {
        exclusive[i] = false;
      }
    }
  }
  vector<size_t> serial_units;
  vector<size_t> parallel_units;
  for (size_t i = 0; i < units.size(); ++i) {
    if (exclusive[i]) {
      parallel_units.push_back(i);
    } else {
      serial_units.push_back(i);
    }
  }
  auto compile_unit = [&](size_t i) -> bool {
    const auto& unit = units[i];
    LOG(INFO) << "compiling dictionary '" << unit.dict_name << "'.";
    try {
      if (!CompileDictionary(unit.dict.get(), unit.compiled_schema_path)) {
        LOG(ERROR) << "dictionary '" << unit.dict_name
                   << "' failed to compile.";
        return false;
      }
    } catch (const std::exception& ex) {
      LOG(ERROR) << "dictionary '" << unit.dict_name
                 << "' threw an exception: " << ex.what();
      return false;
    }
    LOG(INFO) << "dictionary '" << unit.dict_name << "' is ready.";
    return true;
  };
#ifndef RIME_NO_THREADING
  // shared dictionaries first, so that later compilations reuse the built
  // tables and only build their own prisms
  for (size_t i : serial_units) {
    if (compile_unit(i))
      ++success;
    else
      ++failure;
  }
  if (!parallel_units.empty()) {
    LOG(INFO) << "compiling " << parallel_units.size()
              << " dictionaries in parallel.";
    unsigned hw_concurrency =
        (std::max)(1u, std::thread::hardware_concurrency());
    const unsigned kMaxConcurrency = 8;
    hw_concurrency = (std::min)(hw_concurrency, kMaxConcurrency);
    // limit the memory used by one batch to a fraction of physical memory;
    // dictionary compilation is memory heavy (entry collector + trie/prism
    // building), so a conservative estimate per dictionary is used
    uint64_t memory_budget = DetectTotalPhysicalMemory() / 4;
    if (memory_budget == 0) {
      memory_budget = (uint64_t)2 * 1024 * 1024 * 1024;  // fallback 2GB
    }
    const uint64_t kEstimatedBytesPerSourceByte = 16;
    the<ResourceResolver> source_resolver(
        Service::instance().CreateResourceResolver({"source_file", "", ""}));
    vector<pair<uint64_t, size_t>> sized;
    sized.reserve(parallel_units.size());
    for (size_t i : parallel_units) {
      path dict_file =
          source_resolver->ResolvePath(units[i].dict_name + ".dict.yaml");
      uint64_t size = 0;
      std::error_code ec;
      if (auto s = fs::file_size(dict_file, ec); !ec) {
        size = (uint64_t)s;
      }
      sized.push_back({size * kEstimatedBytesPerSourceByte, i});
    }
    sort(sized.begin(), sized.end(),
         [](const auto& a, const auto& b) { return a.first > b.first; });
    size_t begin = 0;
    while (begin < sized.size()) {
      size_t end = begin;
      uint64_t batch_memory = 0;
      unsigned batch_count = 0;
      while (end < sized.size() && batch_count < hw_concurrency) {
        if (batch_memory + sized[end].first > memory_budget) {
          break;
        }
        batch_memory += sized[end].first;
        ++batch_count;
        ++end;
      }
      if (end == begin) {
        ++end;  // a single dictionary larger than the budget: compile alone
      }
      vector<std::future<bool>> futures;
      futures.reserve(end - begin);
      for (size_t k = begin; k < end; ++k) {
        size_t i = sized[k].second;
        futures.push_back(std::async(std::launch::async, [&compile_unit, i]() {
          return compile_unit(i);
        }));
      }
      for (auto& future : futures) {
        if (future.get())
          ++success;
        else
          ++failure;
      }
      begin = end;
    }
  }
#else
  for (size_t i = 0; i < units.size(); ++i) {
    if (compile_unit(i))
      ++success;
    else
      ++failure;
  }
#endif
  LOG(INFO) << "finished updating schemas: " << success << " success, "
            << failure << " failure.";

  the<Config> user_config(Config::Require("user_config")->Create("user"));
  // TODO: store as 64-bit number to avoid the year 2038 problem
  user_config->SetInt("var/last_build_time", (int)time(NULL));

  return failure == 0;
}

SchemaUpdate::SchemaUpdate(TaskInitializer arg) : verbose_(false) {
  try {
    source_path_ = std::any_cast<path>(arg);
  } catch (const std::bad_any_cast&) {
    LOG(ERROR) << "SchemaUpdate: invalid arguments.";
  }
}

static bool RemoveVersionSuffix(string* version, const string& suffix) {
  size_t suffix_pos = version->find(suffix);
  if (suffix_pos != string::npos) {
    version->erase(suffix_pos);
    return true;
  }
  return false;
}

static bool TrashDeprecatedUserCopy(const path& shared_copy,
                                    const path& user_copy,
                                    const string& version_key,
                                    const path& trash) {
  if (!fs::exists(shared_copy) || !fs::exists(user_copy) ||
      fs::equivalent(shared_copy, user_copy)) {
    return false;
  }
  string shared_copy_version;
  string user_copy_version;
  Config shared_config;
  if (shared_config.LoadFromFile(shared_copy)) {
    shared_config.GetString(version_key, &shared_copy_version);
    // treat "X.Y.minimal" as equal to (not greater than) "X.Y"
    // to avoid trashing the user installed full version
    RemoveVersionSuffix(&shared_copy_version, ".minimal");
  }
  Config user_config;
  bool is_customized_user_copy =
      user_config.LoadFromFile(user_copy) &&
      user_config.GetString(version_key, &user_copy_version) &&
      RemoveVersionSuffix(&user_copy_version, ".custom.");
  int cmp = CompareVersionString(shared_copy_version, user_copy_version);
  // rime-installed user copy of the same version should be kept for integrity.
  // also it could have been manually edited by user.
  if (cmp > 0 || (cmp == 0 && is_customized_user_copy)) {
    if (!MaybeCreateDirectory(trash)) {
      return false;
    }
    path backup = trash / user_copy.filename();
    std::error_code ec;
    fs::rename(user_copy, backup, ec);
    if (ec) {
      LOG(ERROR) << "error trashing file " << user_copy;
      return false;
    }
    return true;
  }
  return false;
}

bool SchemaUpdate::Run(Deployer* deployer) {
  if (!fs::exists(source_path_)) {
    LOG(ERROR) << "Error updating schema: nonexistent file '" << source_path_
               << "'.";
    return false;
  }
  string schema_id;
  the<Config> config(new Config);
  if (!config->LoadFromFile(source_path_) ||
      !config->GetString("schema/schema_id", &schema_id) || schema_id.empty()) {
    LOG(ERROR) << "invalid schema definition in '" << source_path_ << "'.";
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
  const path& user_data_path(deployer->user_data_dir);
  if (!MaybeCreateDirectory(deployer->staging_dir)) {
    return false;
  }
  the<ResourceResolver> resolver(
      Service::instance().CreateDeployedResourceResolver(
          {"compiled_schema", "", ".schema.yaml"}));
  auto compiled_schema = resolver->ResolvePath(schema_id);
  if (!CompileDictionary(dict.get(), compiled_schema, verbose_)) {
    LOG(ERROR) << "dictionary '" << dict_name << "' failed to compile.";
    return false;
  }
  LOG(INFO) << "dictionary '" << dict_name << "' is ready.";
  return true;
}

ConfigFileUpdate::ConfigFileUpdate(TaskInitializer arg) {
  try {
    auto p = std::any_cast<pair<string, string>>(arg);
    file_name_ = p.first;
    version_key_ = p.second;
  } catch (const std::bad_any_cast&) {
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
  the<ResourceResolver> resolver(Service::instance().CreateResourceResolver(
      {"config_source_file", "", ".yaml"}));
  for (auto entry : *timestamps.AsMap()) {
    auto value = As<ConfigValue>(entry.second);
    int recorded_time = 0;
    if (!value || !value->GetInt(&recorded_time)) {
      LOG(WARNING) << "invalid timestamp for " << entry.first;
      return true;
    }
    path source_file = resolver->ResolvePath(entry.first);
    if (!fs::exists(source_file)) {
      if (recorded_time) {
        LOG(INFO) << "source file no longer exists: " << source_file;
        return true;
      }
      continue;
    }
    if (recorded_time !=
        (int)filesystem::to_time_t(fs::last_write_time(source_file))) {
      LOG(INFO) << "source file " << (recorded_time ? "changed: " : "added: ")
                << source_file;
      return true;
    }
  }
  return false;
}

bool ConfigFileUpdate::Run(Deployer* deployer) {
  const path shared_data_path(deployer->shared_data_dir);
  const path user_data_path(deployer->user_data_dir);
  // trash deprecated user copy created by an older version of Rime
  path source_config_path(shared_data_path / file_name_);
  path dest_config_path(user_data_path / file_name_);
  path trash = user_data_path / "trash";
  if (TrashDeprecatedUserCopy(source_config_path, dest_config_path,
                              version_key_, trash)) {
    LOG(WARNING) << "deprecated user copy of '" << file_name_
                 << "' is moved to " << trash;
  }
  // build the config file if needs update
  the<Config> config(Config::Require("config")->Create(file_name_));
  if (ConfigNeedsUpdate(config.get())) {
    if (!MaybeCreateDirectory(deployer->staging_dir)) {
      return false;
    }
    config.reset(Config::Require("config_builder")->Create(file_name_));
  }
  return true;
}

bool PrebuildAllSchemas::Run(Deployer* deployer) {
  const path shared_data_path(deployer->shared_data_dir);
  const path user_data_path(deployer->user_data_dir);
  if (!fs::exists(shared_data_path) || !fs::is_directory(shared_data_path))
    return false;
  bool success = true;
  for (fs::directory_iterator iter(shared_data_path), end; iter != end;
       ++iter) {
    path entry(iter->path());
    if (boost::ends_with(entry.filename().u8string(), ".schema.yaml")) {
      the<DeploymentTask> t(new SchemaUpdate(entry));
      if (!t->Run(deployer))
        success = false;
    }
  }
  return success;
}

bool SymlinkingPrebuiltDictionaries::Run(Deployer* deployer) {
  const path shared_data_path(deployer->shared_data_dir);
  const path user_data_path(deployer->user_data_dir);
  if (!fs::exists(shared_data_path) || !fs::is_directory(shared_data_path) ||
      !fs::exists(user_data_path) || !fs::is_directory(user_data_path) ||
      fs::equivalent(shared_data_path, user_data_path))
    return false;
  bool success = false;
  // remove symlinks to shared data files created by previous version
  for (fs::directory_iterator test(user_data_path), end; test != end; ++test) {
    path entry(test->path());
    if (fs::is_symlink(entry)) {
      try {
        // a symlink becomes dangling if the target file is no longer provided
        std::error_code ec;
        auto target_path = fs::canonical(entry, ec);
        bool bad_link = bool(ec);
        bool linked_to_shared_data =
            !bad_link && target_path.has_parent_path() &&
            fs::equivalent(shared_data_path, target_path.parent_path());
        if (bad_link || linked_to_shared_data) {
          LOG(INFO) << "removing symlink: " << entry.filename();
          fs::remove(entry);
        }
      } catch (const fs::filesystem_error& ex) {
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

static bool IsCustomizedCopy(const path& file_path) {
  auto file_name = file_path.filename().u8string();
  if (boost::ends_with(file_name, ".yaml") &&
      !boost::ends_with(file_name, ".custom.yaml")) {
    Config config;
    string checksum;
    if (config.LoadFromFile(file_path) &&
        config.GetString("customization", &checksum)) {
      return true;
    }
  }
  return false;
}

bool BackupConfigFiles::Run(Deployer* deployer) {
  if (!deployer->backup_config_files) {
    LOG(INFO) << "skip backing up config files because it's disabled.";
    return true;
  }
  LOG(INFO) << "backing up config files.";
  const path user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path))
    return false;
  path backup_dir(deployer->user_data_sync_dir());
  if (!MaybeCreateDirectory(backup_dir)) {
    return false;
  }
  int success = 0, failure = 0, latest = 0, skipped = 0;
  for (fs::directory_iterator iter(user_data_path), end; iter != end; ++iter) {
    path entry(iter->path());
    if (!fs::is_regular_file(entry))
      continue;
    auto file_extension = entry.extension().u8string();
    bool is_yaml_file = file_extension == ".yaml";
    bool is_text_file = file_extension == ".txt";
    if (!is_yaml_file && !is_text_file)
      continue;
    path backup = backup_dir / entry.filename();
    if (fs::exists(backup) && Checksum(backup) == Checksum(entry)) {
      ++latest;  // already up-to-date
      continue;
    }
    if (is_yaml_file && IsCustomizedCopy(entry)) {
      ++skipped;  // customized copy
      continue;
    }
    std::error_code ec;
    fs::copy_file(entry, backup, fs::copy_options::overwrite_existing, ec);
    if (ec) {
      LOG(ERROR) << "error backing up file " << backup;
      ++failure;
    } else {
      ++success;
    }
  }
  LOG(INFO) << "backed up " << success << " config files to " << backup_dir
            << ", " << failure << " failed, " << latest << " up-to-date, "
            << skipped << " skipped.";
  return !failure;
}

bool CleanupTrash::Run(Deployer* deployer) {
  LOG(INFO) << "clean up trash.";
  const path user_data_path(deployer->user_data_dir);
  if (!fs::exists(user_data_path))
    return false;
  path trash = user_data_path / "trash";
  int success = 0, failure = 0;
  for (fs::directory_iterator iter(user_data_path), end; iter != end; ++iter) {
    path entry(iter->path());
    if (!fs::is_regular_file(entry))
      continue;
    auto file_name = entry.filename().u8string();
    if (file_name == "rime.log" || boost::ends_with(file_name, ".bin") ||
        boost::ends_with(file_name, ".reverse.kct") ||
        boost::ends_with(file_name, ".userdb.kct.old") ||
        boost::ends_with(file_name, ".userdb.kct.snapshot")) {
      if (!success && !MaybeCreateDirectory(trash)) {
        return false;
      }
      path backup = trash / entry.filename();
      std::error_code ec;
      fs::rename(entry, backup, ec);
      if (ec) {
        LOG(ERROR) << "error clean up file " << entry;
        ++failure;
      } else {
        ++success;
      }
    }
  }
  if (success) {
    LOG(INFO) << "moved " << success << " files to " << trash;
  }
  return !failure;
}

bool CleanOldLogFiles::Run(Deployer* deployer) {
  bool success = true;
#ifdef RIME_ENABLE_LOGGING
  if (FLAGS_logtostderr || FLAGS_log_dir.empty()) {
    return success;
  }

  char ymd[12] = {0};
  time_t now = time(NULL);
  strftime(ymd, sizeof(ymd), ".%Y%m%d", localtime(&now));
  string today(ymd);
  DLOG(INFO) << "today: " << today;

  vector<string> dirs(google::GetLoggingDirectories());

  DLOG(INFO) << "scanning " << dirs.size() << " temp directory for log files.";

  int removed = 0;
  const string& app_name = deployer->app_name;
  for (const auto& dir : dirs) {
    // avoid iteration on non-existing directory, which may cause error
    if (!fs::exists(fs::path(dir)))
      continue;
    vector<path> files_to_remove;
    set<path> files_in_use;
    DLOG(INFO) << "temp directory: " << dir;
    try {
      // preparing files
      for (const auto& entry : fs::directory_iterator(dir)) {
        const string& file_name(entry.path().filename().u8string());
        if (entry.is_regular_file() && !entry.is_symlink() &&
            boost::starts_with(file_name, app_name) &&
            boost::ends_with(file_name, ".log") &&
            !boost::contains(file_name, today)) {
          files_to_remove.push_back(entry.path());
        } else if (entry.is_symlink()) {
          auto target = fs::read_symlink(entry.path());
          const string& target_file_name(target.filename().u8string());
          if (boost::starts_with(target_file_name, app_name) &&
              boost::ends_with(target_file_name, ".log")) {
            files_in_use.insert(target);
          }
        }
      }
    } catch (const fs::filesystem_error& ex) {
      // Catch error to skip up when we have no sufficient permissions.
      // E.g. on Android, there's no read permission on the cwd.
      LOG(WARNING) << "couldn't list directory '" << dir << "': " << ex.what();
      continue;
    }
    // remove files
    for (const auto& file : files_to_remove) {
      if (files_in_use.find(file.filename()) != files_in_use.end())
        continue;
      try {
        DLOG(INFO) << "removing log file '" << file.filename() << "'.";
        // ensure write permission
        fs::permissions(file, fs::perms::owner_write);
        fs::remove(file);
        ++removed;
      } catch (const fs::filesystem_error& ex) {
        LOG(ERROR) << ex.what();
        success = false;
      }
    }
  }
  if (removed != 0) {
    LOG(INFO) << "cleaned " << removed << " log files.";
  }
#endif  // RIME_ENABLE_LOGGING
  return success;
}

}  // namespace rime
