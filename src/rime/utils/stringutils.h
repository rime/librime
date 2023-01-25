//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2023 WhiredPlanck <whiredplanck[at]outlook.com>
//

#ifndef RIME_STRINGUTILS_H_
#define RIME_STRINGUTILS_H_

#include <cstddef>
#include <rime/common.h>
#include <string>

namespace rime {
namespace stringutils {

/// \brief Split the string by delimiter.
inline vector<string> split(const string& str, const string& delimiter, bool token_compress) {
  vector<string> result;
  size_t start = token_compress ? str.find_first_not_of(delimiter, 0) : 0;
  size_t end = str.find_first_of(delimiter, start);

  while (string::npos != end || string::npos != start) {
    result.emplace_back(str.substr(start, end - start));
    
    if (token_compress) {
      start = str.find_first_not_of(delimiter, end);
    } else {
      if (end == string::npos) {
        break;
      }
      start = end + 1;
    }

		end = str.find_first_of(delimiter, start);
	}
  return std::move(result);
}

inline vector<string> split(const string& str, const string& delimiter) {
  return split(str, delimiter, true);
}

} // namespace StringUtils
} // namespace rime

#endif // RIME_STRINGUTILS_H_
