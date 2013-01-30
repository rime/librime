//
// Copyleft 2013 RIME Developers
// License: GPLv3
//
// 2013-01-30 GONG Chen <chen.sst@gmail.com>
//
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include <boost/foreach.hpp>
#include <rime/algo/utilities.h>

namespace rime {

int CompareVersionString(const std::string& x, const std::string& y) {
  if (x.empty() && y.empty()) return 0;
  if (x.empty()) return -1;
  if (y.empty()) return 1;
  std::vector<std::string> xx, yy;
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

uint32_t Checksum(const std::string& file_name) {
  std::ifstream fin(file_name.c_str());
  std::string file_content((std::istreambuf_iterator<char>(fin)),
                           std::istreambuf_iterator<char>());
  boost::crc_32_type crc_32;
  crc_32.process_bytes(file_content.data(), file_content.length());
  return crc_32.checksum();
}

uint32_t Checksum(const std::vector<std::string>& files) {
  boost::crc_32_type crc_32;
  BOOST_FOREACH(const std::string& file_name, files) {
    std::ifstream fin(file_name.c_str());
    std::string file_content((std::istreambuf_iterator<char>(fin)),
                             std::istreambuf_iterator<char>());
    crc_32.process_bytes(file_content.data(), file_content.length());
  }
  return crc_32.checksum();
}

}  // namespace rime
