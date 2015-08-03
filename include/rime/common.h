//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMON_H_
#define RIME_COMMON_H_

#include <forward_list>
#include <memory>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define BOOST_BIND_NO_PLACEHOLDERS
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>

#ifdef RIME_ENABLE_LOGGING
#include <glog/logging.h>
#else
#include "no_logging.h"
#endif  // RIME_ENABLE_LOGGING

// call a pointer to member function on this
#define RIME_THIS_CALL(f) (this->*(f))

namespace rime {

using std::string;
using std::vector;

template <class T>
using list = std::forward_list<T>;
template <class Key, class T>
using map = std::unordered_map<Key, T>;
template <class T>
using set = std::unordered_set<T>;

template <class T>
using the = std::unique_ptr<T>;
template <class T>
using a = std::shared_ptr<T>;
template <class T>
using weak = std::weak_ptr<T>;

template <class X, class Y>
inline a<X> As(const a<Y>& ptr) {
  return std::dynamic_pointer_cast<X>(ptr);
}

template <class X, class Y>
inline bool Is(const a<Y>& ptr) {
  return bool(As<X, Y>(ptr));
}

template <class T, class... Args>
inline a<T> New(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

using boost::signals2::connection;
using boost::signals2::signal;

}  // namespace rime

#endif  // RIME_COMMON_H_
