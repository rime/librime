// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CONTEXT_H_
#define RIME_CONTEXT_H_

#include <rime/common.h>

namespace rime {

// TODO:
class Context {
 public:
  Context() {}
  ~Context() {}

  void set_input(const std::string &value);
  const std::string& input() const { return input_; }

 private:
  std::string input_;
};

}  // namespace rime

#endif  // RIME_CONTEXT_H_
