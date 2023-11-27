#include <rime/algo/strings.h>

namespace rime {
namespace strings {

vector<string> split(const string& str,
                     const string& delim,
                     SplitBehavior behavior) {
  vector<string> strings;
  size_t pos_start = 0, pos_end, delim_len = delim.length();
  while ((pos_end = str.find(delim, pos_start)) != string::npos) {
    if (pos_end != pos_start || behavior != SplitBehavior::SkipToken) {
      string token = str.substr(pos_start, pos_end - pos_start);
      strings.push_back(token);
    }
    pos_start = pos_end + delim_len;
  }
  if (pos_start != str.length() || behavior != SplitBehavior::SkipToken)
    strings.push_back(str.substr(pos_start));
  return strings;
};

vector<string> split(const string& str, const string& delim) {
  return split(str, delim, SplitBehavior::KeepToken);
};

}  // namespace strings
}  // namespace rime
