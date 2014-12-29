//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-21 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SIGNATURE_H_
#define RIME_SIGNATURE_H_

#include <string>

namespace rime {

class Config;
class Deployer;

class Signature {
 public:
  Signature(const std::string& generator, const std::string& key = "signature")
      : generator_(generator), key_(key) {}

  bool Sign(Config* config, Deployer* deployer);

 private:
  std::string generator_;
  std::string key_;
};

}  // namespace rime

#endif  // RIME_SIGNATURE_H_
