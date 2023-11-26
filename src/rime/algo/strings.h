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
  string result;
  if (start != end) {
    result += (*start);
    start++;
  }
  for (; start != end; start++) {
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

}  // namespace strings
}  // namespace rime

#endif  // RIME_STRINGS_H_
