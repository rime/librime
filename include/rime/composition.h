// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-06-19 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMPOSITION_H_
#define RIME_COMPOSITION_H_

#include <string>
#include <vector>
#include <rime/common.h>

namespace rime {

class Menu;

struct Selection {
  enum Manner {
    kOpen,
    kGuessed,
    kSelected,
    kConfirmed,
  };
  Manner manner;
  int index;
  shared_ptr<Menu> menu;
};

class Composition : public std::vector<Selection> {
 public:
  Composition();

  const std::string GetText() const;

};

}  // namespace rime

#endif  // RIME_COMPOSITION_H_
