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
#include <boost/crc.hpp>

namespace rime {

int CompareVersionString(const std::string& x,
                         const std::string& y);

class ChecksumComputer {
 public:
  void ProcessFile(const std::string& file_name);
  uint32_t Checksum();

 private:
  boost::crc_32_type crc_;
};

inline uint32_t Checksum(const std::string& file_name) {
  ChecksumComputer c;
  c.ProcessFile(file_name);
  return c.Checksum();
}

}  // namespace rime

#endif  // RIME_UTILITIES_H_
