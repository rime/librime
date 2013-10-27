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
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <glog/logging.h>

namespace rime {

using boost::signals2::connection;
using boost::signals2::signal;

using boost::scoped_ptr;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::make_shared;

template <class A, class B>
shared_ptr<A> As(const B& ptr) {
  return boost::dynamic_pointer_cast<A>(ptr);
}

template <class A, class B>
bool Is(const B& ptr) {
  return bool(As<A, B>(ptr));
}

template <class T>
shared_ptr<T> New() {
  return boost::make_shared<T>();
}

template <class T, class A>
shared_ptr<T> New(const A& a) {
  return boost::make_shared<T>(a);
}

}  // namespace rime

#endif  // RIME_COMMON_H_
