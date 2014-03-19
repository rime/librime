//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMON_H_
#define RIME_COMMON_H_

#include <memory>
#include <utility>
#define BOOST_BIND_NO_PLACEHOLDERS
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>

#ifdef RIME_ENABLE_LOGGING
#include <glog/logging.h>
#else
#include "no_logging.h"
#endif  // RIME_ENABLE_LOGGGING

namespace rime {

using boost::signals2::connection;
using boost::signals2::signal;

using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;

template <class A, class B>
shared_ptr<A> As(const B& ptr) {
  return std::dynamic_pointer_cast<A>(ptr);
}

template <class A, class B>
bool Is(const B& ptr) {
  return bool(As<A, B>(ptr));
}

template <class T, class... Args>
inline shared_ptr<T> New(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

}  // namespace rime

#endif  // RIME_COMMON_H_
