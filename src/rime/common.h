//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-03-14 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_COMMON_H_
#define RIME_COMMON_H_

#include <rime/build_config.h>

#include <filesystem>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#define BOOST_BIND_NO_PLACEHOLDERS
#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

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
using std::make_unique;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

template <class Key, class T>
using hash_map = boost::unordered_map<Key, T>;
template <class T>
using hash_set = boost::unordered_set<T>;

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

using boost::signals2::connection;
using boost::signals2::signal;

class path : public std::filesystem::path {
  using fs_path = std::filesystem::path;

 public:
  path() : fs_path() {}
  path(const fs_path& p) : fs_path(p) {}
  path(fs_path&& p) : fs_path(std::move(p)) {}
#ifdef _WIN32
  // convert utf-8 string to native encoding path.
  explicit path(const std::string& utf8_path)
      : fs_path(std::filesystem::u8path(utf8_path)) {}
  explicit path(const char* utf8_path)
      : fs_path(std::filesystem::u8path(utf8_path)) {}
#else
  // disable implicit conversion from string to path for development purpose.
  explicit path(const std::string& utf8_path) : fs_path(utf8_path) {}
  explicit path(const char* utf8_path) : fs_path(utf8_path) {}
#endif

  path& operator/=(const path& p) { return *this = fs_path::operator/=(p); }
  path& operator/=(const fs_path& p) { return *this = fs_path::operator/=(p); }
  // convert UTF-8 encoded string to native encoding, then append.
  path& operator/=(const std::string& p) { return *this /= path(p); }
  path& operator/=(const char* p) { return *this /= path(p); }

  friend path operator/(const path& lhs, const path& rhs) {
    return path(lhs) /= rhs;
  }
  friend path operator/(const path& lhs, const fs_path& rhs) {
    return path(lhs) /= rhs;
  }
  friend path operator/(const fs_path& lhs, const path& rhs) {
    return path(lhs) /= rhs;
  }
  // convert UTF-8 encoded string to native encoding, then append.
  friend path operator/(const path& lhs, const std::string& rhs) {
    return path(lhs) /= path(rhs);
  }
  friend path operator/(const path& lhs, const char* rhs) {
    return path(lhs) /= path(rhs);
  }
  friend path operator/(const fs_path& lhs, const std::string& rhs) {
    return path(lhs) /= path(rhs);
  }
  friend path operator/(const fs_path& lhs, const char* rhs) {
    return path(lhs) /= path(rhs);
  }
#ifdef RIME_ENABLE_LOGGING
  friend std::ostream& operator<<(std::ostream& os, const path& p) {
    return os << p.u8string();
  }
#endif
};

}  // namespace rime

#endif  // RIME_COMMON_H_
