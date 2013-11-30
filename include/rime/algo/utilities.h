//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_UTILITIES_H_
#define RIME_UTILITIES_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace rime {

int CompareVersionString(const std::string& x,
                         const std::string& y);

uint32_t Checksum(const std::string& file_name);
uint32_t Checksum(const std::vector<std::string>& files);

}  // namespace rime

#endif  // RIME_UTILITIES_H_
