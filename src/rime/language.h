//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_LANGUAGE_H_
#define RIME_LANGUAGE_H_

#include <rime/common.h>

namespace rime {

class Language {
  const string name_;

 public:
  Language(const string& name) : name_(name) {}
  string name() const { return name_; }

  bool operator== (const Language& other) const {
    return name_ == other.name_;
  }

  template <class T, class U>
  static bool intelligible(const T& t, const U& u) {
    return t && t->language() && u && u->language() &&
        *t->language() == *u->language();
  }

  static string get_language_component(const string& name);
};

}  // namespace rime

#endif  // RIME_LANGUAGE_H_
