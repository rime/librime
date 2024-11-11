#include <rime/algo/strings.h>

namespace rime {
namespace strings {

namespace details {

string ConcatPieces(
    std::initializer_list<pair<const char*, std::size_t>> list) {
  std::size_t size = 0;
  for (auto pair : list) {
    size += pair.second;
  }
  string result;
  result.reserve(size);
  for (const auto& pair : list) {
    result.append(pair.first, pair.first + pair.second);
  }
  assert(result.size() == size);
  return result;
}

}  // namespace details

vector<string> split(const string& str,
                     const string& delim,
                     SplitBehavior behavior) {
  vector<string> strings;
  size_t lastPos, pos;
  if (behavior == SplitBehavior::SkipToken) {
    lastPos = str.find_first_not_of(delim, 0);
  } else {
    lastPos = 0;
  }
  pos = str.find_first_of(delim, lastPos);

  while (std::string::npos != pos || std::string::npos != lastPos) {
    strings.emplace_back(str.substr(lastPos, pos - lastPos));
    if (behavior == SplitBehavior::SkipToken) {
      lastPos = str.find_first_not_of(delim, pos);
    } else {
      if (pos == std::string::npos) {
        break;
      }
      lastPos = pos + 1;
    }
    pos = str.find_first_of(delim, lastPos);
  }
  return strings;
};

vector<string> split(const string& str, const string& delim) {
  return split(str, delim, SplitBehavior::KeepToken);
};

}  // namespace strings
}  // namespace rime
