// rime_patch.cc
// Foodgen <chen.sst@gmail.com>
//
#include <iostream>
#include <rime_api.h>
#include <rime_levers_api.h>
#include <rime/common.h>

// usage:
//   rime_patch config_id key [yaml]
// example:
//   rime_patch default "menu/page_size" 9
//   rime_patch default schema_list/@0/schema combo_pinyin
//   rime_patch default schema_list '[{schema: luna_pinyin}]'
//   rime_patch default schema_list  # read yaml from stdin

using namespace rime;

int apply_patch(const string& config_id,
                const string& key, const string& yaml) {
  RimeApi* rime = rime_get_api();
  RimeModule* module = rime->find_module("levers");
  if (!module) {
    std::cerr << "missing Rime module: levers" << std::endl;
    return 1;
  }
  RimeLeversApi* levers = (RimeLeversApi*)module->get_api();

  int ret = 1;

  RimeConfig value = {0};  // should be zero-initialized
  if (rime->config_load_string(&value, yaml.c_str())) {

    RimeCustomSettings* settings =
        levers->custom_settings_init(config_id.c_str(), "rime_patch");
    levers->load_settings(settings);

    if (levers->customize_item(settings, key.c_str(), &value)) {
      levers->save_settings(settings);
      std::cerr << "patch applied." << std::endl;
      ret = 0;
    }

    levers->custom_settings_destroy(settings);
    rime->config_close(&value);
  }
  else {
    std::cerr << "bad yaml document." << std::endl;
  }

  return ret;
}

int main(int argc, char *argv[]) {
  if (argc < 3 || argc > 4) {
    std::cerr << "usage: " << argv[0] << " config_id key [yaml]" << std::endl;
    return 1;
  }

  RimeApi* rime = rime_get_api();

  RIME_STRUCT(RimeTraits, traits);
  traits.app_name = "rime.patch";
  rime->setup(&traits);

  rime->deployer_initialize(&traits);

  string config_id(argv[1]);
  string key(argv[2]);
  string yaml;
  if (argc > 3) {
    yaml.assign(argv[3]);
  }
  else {
    // read yaml string from stdin
    yaml.assign((std::istreambuf_iterator<char>(std::cin)),
                std::istreambuf_iterator<char>());
  }
  int ret = apply_patch(config_id, key, yaml);

  rime->finalize();

  return ret;
}
