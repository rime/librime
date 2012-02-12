// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-02-12 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_CUSTOMIZER_H_
#define RIME_CUSTOMIZER_H_

#include <string>
#include <boost/filesystem.hpp>

namespace rime {

class Customizer {
 public:
  Customizer(const boost::filesystem::path& source_path,
             const boost::filesystem::path& dest_path,
             const std::string& version_key)
      : source_path_(source_path),
        dest_path_(dest_path),
        version_key_(version_key) {}
  
  bool UpdateConfigFile();

 protected:
  static int CompareVersionString(const std::string& x,
                                  const std::string& y);
  
  boost::filesystem::path source_path_;
  boost::filesystem::path dest_path_;
  std::string version_key_;
};  

}  // namespace rime

#endif  // RIME_CUSTOMIZER_H_
