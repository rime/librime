// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMON_H_
#define RIME_COMMON_H_

#include <cstdlib>
#include <string>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

#include <glog/logging.h>

namespace rime {

using boost::scoped_ptr;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::make_shared;

template <class A, class B>
shared_ptr<A> As(const B &ptr) {
  return boost::dynamic_pointer_cast<A>(ptr);
}

void RegisterComponents();

}  // namespace rime

#endif  // RIME_COMMON_H_
