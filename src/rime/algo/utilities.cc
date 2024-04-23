//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <sstream>
#include <rime/algo/utilities.h>

namespace rime {

int CompareVersionString(const string& x, const string& y) {
  size_t i = 0, j = 0, m = x.size(), n = y.size();
  while (i < m || j < n) {
    int v1 = 0, v2 = 0;
    while (i < m && x[i] != '.')
      v1 = v1 * 10 + (int)(x[i++] - '0');
    ++i;
    while (j < n && y[j] != '.')
      v2 = v2 * 10 + (int)(y[j++] - '0');
    ++j;
    if (v1 > v2)
      return 1;
    if (v1 < v2)
      return -1;
  }
  return 0;
}

ChecksumComputer::ChecksumComputer(uint32_t initial_remainder)
    : crc_(initial_remainder) {}

void ChecksumComputer::ProcessFile(const path& file_path) {
  std::ifstream fin(file_path.c_str());
  std::stringstream buffer;
  buffer << fin.rdbuf();
  const auto& file_content(buffer.str());
  crc_.process_bytes(file_content.data(), file_content.length());
}

uint32_t ChecksumComputer::Checksum() {
  return crc_.checksum();
}

}  // namespace rime
