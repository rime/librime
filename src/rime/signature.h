//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-02-21 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SIGNATURE_H_
#define RIME_SIGNATURE_H_

#include <rime/config.h>
#include <rime/deployer.h>

namespace rime {

class Signature {
 public:
  Signature(string_view generator, string_view key = "signature"sv)
      : generator_(generator), key_(key) {}

  bool Sign(Config* config, Deployer* deployer);

 private:
  string generator_;
  string key_;
};

}  // namespace rime

#endif  // RIME_SIGNATURE_H_
