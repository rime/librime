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

}  // namespace rime

#endif  // RIME_UTILITIES_H_
