// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MENU_H_
#define RIME_MENU_H_

#include <vector>
#include <rime/common.h>

namespace rime {

class Translation;
  
class Menu {
 public:
  Menu() {}
  ~Menu() {}

  void AddTranslation(shared_ptr<Translation> translation);

 private:
  std::vector<shared_ptr<Translation> > translations_;
};

}  // namespace rime

#endif  // RIME_MENU_H_
