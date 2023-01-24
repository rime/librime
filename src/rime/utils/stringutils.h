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
inline vector<string> split(const string& str, const string& delimiter) {
  vector<string> result;
  size_t start = 0;
  size_t end = str.find_first_of(delimiter);

  while (string::npos >= end) {
    result.emplace_back(str.substr(start, end - start));
        
    if (string::npos == end) break;

		start = end + 1;
		end = str.find_first_of(delimiter, start);
	}
  return std::move(result);
}

} // namespace StringUtils
} // namespace rime

#endif // RIME_STRINGUTILS_H_
