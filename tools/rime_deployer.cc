//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-07-07 GONG Chen <chen.sst@gmail.com>
//
#include <iostream>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/service.h>
#include <rime/setup.h>
#include <rime/lever/deployment_tasks.h>
#include "codepage.h"

using namespace rime;

int add_schema(int count, char* schemas[]) {
  Config config;
  if (!config.LoadFromFile(path{"default.custom.yaml"})) {
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
    if (already_there)
      continue;
    schema_list[schema_list.size()]["schema"] = new_schema_id;
    LOG(INFO) << "added schema: " << new_schema_id;
  }
  if (!config.SaveToFile(path{"default.custom.yaml"})) {
    LOG(ERROR) << "failed to save schema list.";
    return 1;
  }
  return 0;
}

int set_active_schema(const string& schema_id) {
  Config config;
  if (!config.LoadFromFile(path{"user.yaml"})) {
    LOG(INFO) << "creating new file 'user.yaml'.";
  }
  config["var"]["previously_selected_schema"] = schema_id;
  if (!config.SaveToFile(path{"user.yaml"})) {
    LOG(ERROR) << "failed to set active schema: " << schema_id;
    return 1;
  }
  return 0;
}

static void setup_deployer(Deployer* deployer, int argc, char* argv[]) {
  if (argc > 0) {
    deployer->user_data_dir = path(argv[0]);
  }
  if (argc > 1) {
    deployer->shared_data_dir = path(argv[1]);
  } else if (argc > 0) {
    deployer->shared_data_dir = path(argv[0]);
  }
  if (argc > 2) {
    deployer->staging_dir = path(argv[2]);
  } else {
    deployer->staging_dir = deployer->user_data_dir / "build";
  }
  deployer->prebuilt_data_dir = deployer->shared_data_dir / "build";
}

int main(int argc, char* argv[]) {
  unsigned int codepage = SetConsoleOutputCodePage();
  SetupLogging("rime.tools");

  if (argc == 1) {
    std::cout
        << "Usage: " << std::endl
        << "\t--add-schema <schema_id>..." << std::endl
        << "\t\tAdd one or more schema_id(s) to the schema_list, write patch "
           "in default.custom.yaml"
        << std::endl
        << std::endl
        << "\t--build [user_data_dir] [shared_data_dir] [staging_dir]"
        << std::endl
        << "\t\tBuild and deploy Rime data." << std::endl
        << "\t\tIf unspecified, user_data_dir and shared_data_dir defaults to "
           "the working directory."
        << std::endl
        << "\t\tTo deploy data for ibus-rime, use the following directories:"
        << std::endl
        << "\t\tuser_data_dir    ~/.config/ibus/rime" << std::endl
        << "\t\tshared_data_dir  /usr/share/rime-data" << std::endl
        << "\t\tstaging_dir      ~/.config/ibus/rime/build" << std::endl
        << std::endl
        << "\t--compile <x.schema.yaml> [user_data_dir] [shared_data_dir] "
           "[staging_dir]"
        << std::endl
        << "\t\tCompile a specific schema's dictionary files." << std::endl
        << std::endl
        << "\t--set-active-schema <schema_id>" << std::endl
        << "\t\tSet the active schema in user.yaml" << std::endl;

    SetConsoleOutputCodePage(codepage);
    return 0;
  }

  string option;
  if (argc >= 2)
    option = argv[1];
  // shift
  argc -= 2, argv += 2;

  if (argc >= 0 && argc <= 3 && option == "--build") {
    Deployer& deployer(Service::instance().deployer());
    setup_deployer(&deployer, argc, argv);
    LoadModules(kDeployerModules);
    WorkspaceUpdate update;
    int res = update.Run(&deployer) ? 0 : 1;
    SetConsoleOutputCodePage(codepage);
    return res;
  }

  if (argc >= 1 && option == "--add-schema") {
    int res = add_schema(argc, argv);
    SetConsoleOutputCodePage(codepage);
    return res;
  }

  if (argc == 1 && option == "--set-active-schema") {
    int res = set_active_schema(argv[0]);
    SetConsoleOutputCodePage(codepage);
    return res;
  }

  if (argc >= 1 && option == "--compile") {
    Deployer& deployer(Service::instance().deployer());
    setup_deployer(&deployer, argc - 1, argv + 1);
    LoadModules(kDeployerModules);
    path schema_file(argv[0]);
    SchemaUpdate update(schema_file);
    update.set_verbose(true);
    int res = update.Run(&deployer) ? 0 : 1;
    SetConsoleOutputCodePage(codepage);
    return res;
  }

  std::cerr << "invalid arguments." << std::endl;
  SetConsoleOutputCodePage(codepage);
  return 1;
}
