#ifndef RIME_STRINGS_H_
#define RIME_STRINGS_H_

#include <rime/common.h>
#include <initializer_list>

namespace rime {
namespace strings {

enum class SplitBehavior { KeepToken, SkipToken };

vector<string> split(const string& str,
                     const string& delim,
                     SplitBehavior behavior);

vector<string> split(const string& str, const string& delim);

template <typename Iter, typename T>
string join(Iter start, Iter end, T&& delim) {
  string result = (*start);
  while (++start != end) {
    result += (delim);
    result += (*start);
  }
  return result;
}

template <typename C, typename T>
inline string join(C&& container, T&& delim) {
  return join(std::begin(container), std::end(container), delim);
}

template <typename C, typename T>
inline string join(std::initializer_list<C>&& container, T&& delim) {
  return join(std::begin(container), std::end(container), delim);
}

inline void trim_left(string& str) {
  str.erase(str.begin(),
            std::find_if(str.begin(), str.end(),
                         [](unsigned char ch) { return !std::isspace(ch); }));
}

inline void trim_right(string& str) {
  str.erase(std::find_if(str.rbegin(), str.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            str.end());
}

inline void trim(string& str) {
  trim_right(str);
  trim_left(str);
}

inline bool starts_with(const string& str, const string& prefix) {
  return str.rfind(prefix, 0) == 0;
}

inline bool ends_with(const string& str, const string& suffix) {
  return str.find(suffix, str.length() - suffix.length()) != std::string::npos;
}

}  // namespace strings
}  // namespace rime

#endif  // RIME_STRINGS_H_
