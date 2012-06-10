// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include <boost/lexical_cast.hpp>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/expl/customizer.h>

namespace fs = boost::filesystem;

static uint32_t checksum(const std::string &file_name) {
  std::ifstream fin(file_name.c_str());
  std::string file_content((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
  boost::crc_32_type crc_32;
  crc_32.process_bytes(file_content.data(), file_content.length());
  return crc_32.checksum();
}

namespace rime {

int CompareVersionString(const std::string& x, const std::string& y);

// update strategy:
//
// source custom dest
// 1.0    -      1.0 (up-to-date)
// 2.0    -      1.0  --> Update: 2.0
// 1.0    X      1.0  --> Update: 1.0.custom.X
// 2.0    X      1.0  --> Update: 2.0.custom.X
// 1.0    X      1.0.custom.X (up-to-date)
// 2.0    X      1.0.custom.X  --> Update: 2.0.custom.X
// 1.0    X      2.0  --> Update: 2.0.custom.X
// 1.0    X      2.0.custom.X (up-to-date)
// 1.0    Y      1.0.custom.X  --> Update: 1.0.custom.Y
//
bool Customizer::UpdateConfigFile() {
  bool need_update = false;
  bool redistribute = false;
  std::string source_version;
  std::string dest_version;
  std::string applied_customization;

  Config dest_config;
  if (dest_config.LoadFromFile(dest_path_.string())) {
    dest_config.GetString(version_key_, &dest_version);
    dest_config.GetString("customization", &applied_customization);
  }
  if (fs::exists(source_path_) &&
      fs::exists(dest_path_) &&
      fs::equivalent(source_path_, dest_path_)) {
    source_version = dest_version;
  }
  else {
    Config source_config;
    if (source_config.LoadFromFile(source_path_.string())) {
      source_config.GetString(version_key_, &source_version);
    }
    else {
      EZLOGGERPRINT("Error loading config from '%s'.",
                    source_path_.string().c_str());
      return false;
    }
    if (CompareVersionString(source_version, dest_version) > 0) {
      need_update = true;
      redistribute = true;
    }
  }
  
  fs::path custom_path(dest_path_);
  if (custom_path.extension() != ".yaml") {
    custom_path.clear();
  }
  else {
    custom_path.replace_extension();
    if (custom_path.extension() == ".schema") {
      custom_path.replace_extension();
    }
    custom_path = custom_path.string() + ".custom.yaml";
  }
  std::string customization;
  if (!custom_path.empty() && fs::exists(custom_path)) {
    customization = boost::lexical_cast<std::string>(
        checksum(custom_path.string()));
  }
  if (applied_customization != customization) {
    need_update = true;
  }

  if (!need_update) {
    EZLOGGERPRINT("config file '%s' is up-to-date.",
                  dest_path_.string().c_str());
    return false;
  }
  EZLOGGERPRINT("updating config file '%s'.", dest_path_.string().c_str());

  if (redistribute ||
      !applied_customization.empty()) {
    try {
      fs::copy_file(source_path_, dest_path_,
                    fs::copy_option::overwrite_if_exists);
    }
    catch (...) {
      EZLOGGERPRINT("Error copying config file '%s' to user directory.",
                    source_path_.string().c_str());
      return false;
    }
  }
  if (!customization.empty()) {
    EZLOGGERPRINT("applying customization from '%s'.",
                  custom_path.string().c_str());
    if (!dest_config.LoadFromFile(dest_path_.string())) {
      EZLOGGERPRINT("Error reloading destination config file.");
      return false;
    }
    // applying patch
    Config custom_config;
    if (!custom_config.LoadFromFile(custom_path.string())) {
      EZLOGGERPRINT("Error loading customization file.");
      return false;
    }
    ConfigMapPtr patch = custom_config.GetMap("patch");
    if (!patch) {
      EZLOGGERPRINT("Warning: 'patch' not found in customization file.");
    }
    else {
      for (ConfigMap::Iterator it = patch->begin(); it != patch->end(); ++it) {
        if (!dest_config.SetItem(it->first, it->second)) {
          EZLOGGERPRINT("Error applying customization item: '%s'.",
                        it->first.c_str());
          return false;
        }
      }
    }
    // update config version
    dest_config.GetString(version_key_, &dest_version);
    dest_config.SetString(version_key_,
                          dest_version + ".custom." + customization);
    dest_config.SetString("customization", customization);
    if (!dest_config.SaveToFile(dest_path_.string())) {
      EZLOGGERPRINT("Error saving destination config file.");
      return false;
    }
  }

  return true;
}

}  // namespace rime
