// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
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
  explicit Signature(const std::string& generator)
      : generator_(generator) {}
  bool Sign(Config* config, Deployer* deployer);
 private:
  std::string generator_;
};

}  // namespace rime

#endif  // RIME_SIGNATURE_H_
