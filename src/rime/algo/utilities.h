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

int CompareVersionString(const string& x, const string& y);

class ChecksumComputer {
 public:
  explicit ChecksumComputer(uint32_t initial_remainder = 0);
  void ProcessFile(const path& file_path);
  uint32_t Checksum();

 private:
  boost::crc_32_type crc_;
};

inline uint32_t Checksum(const path& file_path) {
  ChecksumComputer c;
  c.ProcessFile(file_path);
  return c.Checksum();
}

}  // namespace rime

#endif  // RIME_UTILITIES_H_
