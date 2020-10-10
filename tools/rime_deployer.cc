//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-07-07 GONG Chen <chen.sst@gmail.com>
//
#include <iostream>
#include <boost/filesystem.hpp>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/service.h>
#include <rime/setup.h>
#include <rime/lever/deployment_tasks.h>

namespace fs = boost::filesystem;

using namespace rime;

int add_schema(int count, char* schemas[]) {
  Config config;
  if (!config.LoadFromFile("default.custom.yaml")) {
    LOG(INFO) << "creating new file 'default.custom.yaml'.";
  }
  ConfigMapEntryRef schema_list(config["patch"]["schema_list"]);
  for (int i = 0; i < count; ++i) {
    if (!schemas[i])
      return 1;
    string new_schema_id(schemas[i]);
    bool already_there = false;
    for (size_t j = 0; j < schema_list.size(); ++j) {
      if (!schema_list[j].HasKey("schema"))
        continue;
      string schema_id(schema_list[j]["schema"].ToString());
      if (schema_id == new_schema_id) {
        already_there = true;
        break;
      }
    }
    if (already_there) continue;
    schema_list[schema_list.size()]["schema"] = new_schema_id;
    LOG(INFO) << "added schema: " << new_schema_id;
  }
  if (!config.SaveToFile("default.custom.yaml")) {
    LOG(ERROR) << "failed to save schema list.";
    return 1;
  }
  return 0;
}

int set_active_schema(const string& schema_id) {
  Config config;
  if (!config.LoadFromFile("user.yaml")) {
    LOG(INFO) << "creating new file 'user.yaml'.";
  }
  config["var"]["previously_selected_schema"] = schema_id;
  if (!config.SaveToFile("user.yaml")) {
    LOG(ERROR) << "failed to set active schema: " << schema_id;
    return 1;
  }
  return 0;
}

static void setup_deployer(Deployer* deployer,
                           int argc, char* argv[]) {
  if (argc > 0) {
    deployer->user_data_dir = argv[0];
  }
  if (argc > 1) {
    deployer->shared_data_dir = argv[1];
  } else if (argc > 0) {
    deployer->shared_data_dir = argv[0];
  }
  if (argc > 2) {
    deployer->staging_dir = argv[2];
  } else {
    deployer->staging_dir = (fs::path(deployer->user_data_dir) / "build").string();
  }
  deployer->prebuilt_data_dir = (fs::path(deployer->shared_data_dir) / "build").string();
}

int main(int argc, char* argv[]) {
  SetupLogging("rime.tools");

  if (argc == 1) {
    std::cout << "Usage: " << std::endl
              << "\t--add-schema [schema_id] ..." << std::endl
              << "\t\tAdd one or more schema_id to the schema_list in default.custom.yaml" << std::endl
              << std::endl
              << "\t-build [user_data_dir] [shared_data_dir] [built_target_dir]" << std::endl
              << "\t\tManually build & deploy, the dirs have to be specified." << std::endl
              << "\t\tFor linux, the default dirs are " << std::endl
              << "\t\t[user_data_dir]     ~/.config/ibus/rime/" << std::endl
              << "\t\t[shared_data_dir]   /usr/share/rime-data/" << std::endl
              << "\t\t[built_target_dir]  ~/.config/ibus/rime/build" << std::endl
              << std::endl
              << "\t--compile [x.schema.yaml] <user_data_dir> <shared_data_dir> <built_target_dir>" << std::endl
              << "\t\tManually compile a schema's .dict file." << std::endl
              << std::endl
              << "\t--set-active-schema [schema_id]" << std::endl
              << "\t\tModify previously_selected_schema in user.yaml" << std::endl
      ;

    return 0;
  }

  string option;
  if (argc >= 2) option = argv[1];
  // shift
  argc -= 2, argv += 2;

  if (argc >= 0 && argc <= 3 && option == "--build") {
    Deployer& deployer(Service::instance().deployer());
    setup_deployer(&deployer, argc, argv);
    LoadModules(kDeployerModules);
    WorkspaceUpdate update;
    return update.Run(&deployer) ? 0 : 1;
  }

  if (argc >= 1 && option == "--add-schema") {
    return add_schema(argc, argv);
  }

  if (argc == 1 && option == "--set-active-schema") {
    return set_active_schema(argv[0]);
  }

  if (argc >= 1 && option == "--compile") {
    Deployer& deployer(Service::instance().deployer());
    setup_deployer(&deployer, argc - 1, argv + 1);
    LoadModules(kDeployerModules);
    string schema_file(argv[0]);
    SchemaUpdate update(schema_file);
    update.set_verbose(true);
    return update.Run(&deployer) ? 0 : 1;
  }

  std::cerr << "invalid arguments." << std::endl;
  return 1;
}
