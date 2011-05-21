// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#ifndef RIME_TRANSLATION_H_
#define RIME_TRANSLATION_H_

#include <string>
#include <rime/context.h>

namespace rime {
  
class Translation {
 public:
  Translation();
  virtual ~Translation();
  
  void set_result(const std::string &value) {
    result_ = value;
  }
  const std::string& result() const {
    return result_;
  };

 private:
  // For testing
  std::string result_;
};

} // namespace rime

#endif  // RIME_TRANSLATION_H_
