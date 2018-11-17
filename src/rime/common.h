//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMON_H_
#define RIME_COMMON_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#define BOOST_BIND_NO_PLACEHOLDERS
#ifdef BOOST_SIGNALS2
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#else
#include <boost/signals.hpp>
#endif

#ifdef RIME_ENABLE_LOGGING
#include <glog/logging.h>
#else
#include "no_logging.h"
#endif  // RIME_ENABLE_LOGGING

// call a pointer to member function on this
#define RIME_THIS_CALL(f) (this->*(f))

#define RIME_THIS_CALL_AS(T, f) ((T*)this->*(f))

namespace rime {

using std::function;
using std::list;
using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;
using boost::optional;

template <class Key, class T>
using hash_map = std::unordered_map<Key, T>;
template <class T>
using hash_set = std::unordered_set<T>;

template <class T>
using the = std::unique_ptr<T>;
template <class T>
using an = std::shared_ptr<T>;
template <class T>
using of = an<T>;
template <class T>
using weak = std::weak_ptr<T>;

template <class X, class Y>
inline an<X> As(const an<Y>& ptr) {
  return std::dynamic_pointer_cast<X>(ptr);
}

template <class X, class Y>
inline bool Is(const an<Y>& ptr) {
  return bool(As<X, Y>(ptr));
}

template <class T, class... Args>
inline an<T> New(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

#ifdef BOOST_SIGNALS2
using boost::signals2::connection;
using boost::signals2::signal;
#else
using boost::signals::connection;
using boost::signal;
#endif

}  // namespace rime

#endif  // RIME_COMMON_H_
