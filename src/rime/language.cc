//
// Copyright RIME Developers
// Distributed under the BSD License
//
#include <rime/common.h>
#include <rime/language.h>

namespace rime {

// "luna_pinyin.extra" has language component "luna_pinyin".
string Language::get_language_component(string_view name) {
  size_t dot = name.find('.');
  if (dot != string::npos && dot != 0)
    return string(name.substr(0, dot));
  return string(name);
}

}  // namespace rime
