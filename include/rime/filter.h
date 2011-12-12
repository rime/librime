// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-11 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_FILTER_H_
#define RIME_FILTER_H_

#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/component.h>

namespace rime {

class Engine;

class Filter : public Class<Filter, Engine*> {
 public:
  Filter(Engine *engine) : engine_(engine) {}
  virtual ~Filter() {}

  virtual bool Proceed(CandidateList *recruited,
                       CandidateList *candidates) = 0;

 protected:
  Engine *engine_;
};

}  // namespace rime

#endif  // RIME_FILTER_H_


