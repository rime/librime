#ifndef RIME_FS_H_
#define RIME_FS_H_

#include <chrono>

namespace rime {
namespace filesystem {

template <typename TP>
inline std::time_t to_time_t(TP tp) {
  using namespace std::chrono;
  auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() +
                                                      system_clock::now());
  return system_clock::to_time_t(sctp);
}

}  // namespace filesystem
}  // namespace rime

#endif  // RIME_FS_H_