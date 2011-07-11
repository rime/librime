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
#include <rime/segmentation.h>

namespace rime {

class Composition : public Segmentation {
 public:
  Composition();
  const std::string GetText() const;
};

}  // namespace rime

#endif  // RIME_COMPOSITION_H_
