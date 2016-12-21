//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-15 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SEGMENTOR_H_
#define RIME_SEGMENTOR_H_

#include <rime/component.h>
#include <rime/ticket.h>

namespace rime {

class Engine;
class Segmentation;

class Segmentor : public Class<Segmentor, const Ticket&> {
 public:
  explicit Segmentor(const Ticket& ticket)
      : engine_(ticket.engine), name_space_(ticket.name_space) {}
  virtual ~Segmentor() = default;

  virtual bool Proceed(Segmentation* segmentation) = 0;

 protected:
  Engine* engine_;
  string name_space_;
};

}  // namespace rime

#endif  // RIME_SEGMENTOR_H_
