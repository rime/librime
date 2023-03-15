//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <rime/algo/utilities.h>

namespace rime {

int CompareVersionString(const string& x, const string& y) {
  if (x.empty() && y.empty()) return 0;
  if (x.empty()) return -1;
  if (y.empty()) return 1;
  vector<string> xx, yy;
  boost::split(xx, x, boost::is_any_of("."));
  boost::split(yy, y, boost::is_any_of("."));
  size_t i = 0;
  for (; i < xx.size() && i < yy.size(); ++i) {
    int dx = atoi(xx[i].c_str());
    int dy = atoi(yy[i].c_str());
    if (dx != dy) return dx - dy;
    int c = xx[i].compare(yy[i]);
    if (c != 0) return c;
  }
  if (i < xx.size()) return 1;
  if (i < yy.size()) return -1;
  return 0;
}

ChecksumComputer::ChecksumComputer(uint32_t initial_remainder)
    : crc_(initial_remainder) {}

void ChecksumComputer::ProcessFile(const string& file_name) {
    auto ftime = boost::filesystem::last_write_time(file_name);
    auto fsize = boost::filesystem::file_size(file_name);
    crc_.process_bytes((const void*) &ftime, sizeof(ftime));
    crc_.process_bytes((const void*) &fsize, sizeof(fsize));
}

uint32_t ChecksumComputer::Checksum() {
  return crc_.checksum();
}

}  // namespace rime
