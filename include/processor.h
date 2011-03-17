// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_PROCESSOR_H_
#define RIME_PROCESSOR_H_

#include "component.h"

namespace rime
{

class Kevent;

class Processor : Component
{
 public:
  virtual bool ProcessKeyEvent(const Kevent& /*kevent*/)
  {
    return false;
  }
};

}  // namespace rime

#endif  // RIME_PROCESSOR_H_
