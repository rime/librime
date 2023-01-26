//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UTILITIES_H_
#define RIME_UTILITIES_H_

#include <stdint.h>
#include <boost/crc.hpp>
#include <rime/common.h>

namespace rime {

int CompareVersionString(const string& x,
                         const string& y);

class ChecksumComputer {
 public:
  explicit ChecksumComputer(uint32_t initial_remainder = 0);
  void ProcessFile(const string& file_name);
  uint32_t Checksum();

 private:
  boost::crc_32_type crc_;
};

inline uint32_t Checksum(const string& file_name) {
  ChecksumComputer c;
  c.ProcessFile(file_name);
  return c.Checksum();
}

namespace StringUtils {

/// \brief Split the string by delimiter.
inline vector<string> Split(const string& str, const string& delimiter, bool token_compress) {
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

inline vector<string> Split(const string& str, const string& delimiter) {
  return Split(str, delimiter, true);
}

/// \brief Join a range of string with delim.
template <typename Iter, typename T>
inline string Join(Iter start, Iter end, T&& delim) {
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

/// \brief Join a set of string with delim.
template <typename C, typename T>
inline string Join(C&& container, T&& delim) {
    return Join(std::begin(container), std::end(container), delim);
}

/// \brief Join the strings with delim.
template <typename C, typename T>
inline string Join(std::initializer_list<C>&& container, T&& delim) {
    return Join(std::begin(container), std::end(container), delim);
}


} // namespace StringUtils

}  // namespace rime

#endif  // RIME_UTILITIES_H_
