//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <rime/algo/utilities.h>

namespace rime {

int CompareVersionString(const string& x, const string& y) {
  if (x.empty() && y.empty())
    return 0;
  if (x.empty())
    return -1;
  if (y.empty())
    return 1;
  vector<string> xx, yy;
  boost::split(xx, x, boost::is_any_of("."));
  boost::split(yy, y, boost::is_any_of("."));
  size_t i = 0;
  for (; i < xx.size() && i < yy.size(); ++i) {
    int dx = atoi(xx[i].c_str());
    int dy = atoi(yy[i].c_str());
    if (dx != dy)
      return dx - dy;
    int c = xx[i].compare(yy[i]);
    if (c != 0)
      return c;
  }
  if (i < xx.size())
    return 1;
  if (i < yy.size())
    return -1;
  return 0;
}

ChecksumComputer::ChecksumComputer(uint32_t initial_remainder)
    : crc_(initial_remainder) {}

void ChecksumComputer::ProcessFile(const string& file_name) {
  std::ifstream fin(file_name.c_str());
  std::stringstream buffer;
  buffer << fin.rdbuf();
  const auto& file_content(buffer.str());
  crc_.process_bytes(file_content.data(), file_content.length());
}

uint32_t ChecksumComputer::Checksum() {
  return crc_.checksum();
}

}  // namespace rime
