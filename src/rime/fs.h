#ifndef RIME_FS_H_
#define RIME_FS_H_

#include <chrono>

#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L  // std::filesystem
#include <filesystem>

namespace rime {

namespace fs {
using namespace std::filesystem;
using system_error_code = std::error_code;

template <typename TP>
inline std::time_t to_time_t(const TP& tp) {
  using namespace std::chrono;
  auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() +
                                                      system_clock::now());
  return system_clock::to_time_t(sctp);
};

inline std::time_t last_write_time(const fs::path& tp) {
  return to_time_t(std::filesystem::last_write_time(tp));
}
inline fs::path absolute(const fs::path& path, const fs::path& root_path) {
  return absolute(root_path / path);
}
inline fs::path absolute(const fs::path& path) {
  return std::filesystem::absolute(path);
}

inline fs::path system_complete(const fs::path& path) {
  return std::filesystem::absolute(path);
}
}  // namespace fs

}  // namespace rime

#else  // boost::filesystem

#include <boost/filesystem.hpp>

namespace rime {
namespace fs {
using namespace boost::filesystem;
using system_error_code = boost::system::error_code;

}  // namespace fs

}  // namespace rime
#endif  // __cplusplus  C++17
#endif  // RIME_FS_H_
