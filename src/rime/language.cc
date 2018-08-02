//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/common.h>
#include <rime/language.h>

namespace rime {

// "luna_pinyin.extra" has language component "luna_pinyin".
string Language::get_language_component(const string& name) {
  size_t dot = name.find('.');
  if (dot != string::npos && dot != 0)
    return name.substr(0, dot);
  return name;
}

}  // namespace rime
