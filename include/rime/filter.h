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
#include <rime/ticket.h>

namespace rime {

class Engine;
struct Segment;

class Filter : public Class<Filter, const Ticket&> {
 public:
  explicit Filter(const Ticket& ticket)
      : engine_(ticket.engine), name_space_(ticket.name_space) {}
  virtual ~Filter() {}

  virtual void Apply(CandidateList *recruited,
                     CandidateList *candidates) = 0;

  virtual bool AppliesToSegment(Segment* segment) {
    return true;
  }

 protected:
  Engine *engine_;
  std::string name_space_;
};

}  // namespace rime

#endif  // RIME_FILTER_H_
