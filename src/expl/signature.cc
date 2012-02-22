// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-02-21 GONG Chen <chen.sst@gmail.com>
//
#include <time.h>
#include <boost/algorithm/string.hpp>
#include <rime_version.h>
#include <rime/config.h>
#include <rime/deployer.h>
#include <rime/expl/signature.h>

namespace rime {

bool Signature::Sign(Config* config, Deployer* deployer) {
  if (!config) return false;
  config->SetString("customization/generator", generator_);
  time_t now = time(NULL);
  std::string time_str(ctime(&now));
  boost::trim(time_str);
  config->SetString("customization/modified_time", time_str);
  config->SetString("customization/distribution_code_name",
                    deployer->distribution_code_name);
  config->SetString("customization/distribution_version",
                    deployer->distribution_version);
  config->SetString("customization/rime_version",
                    RIME_VERSION);
  return true;
}

}  // namespace rime
