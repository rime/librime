//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_REWRITE_BINARY_H_
#define RIME_REWRITE_BINARY_H_

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <ostream>
#include <system_error>
#include <utility>

#include <boost/align/align_up.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <rime/common.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace rime {
namespace rewrite_internal {

inline uint32_t ReadU32(const char* p) {
  return boost::endian::endian_load<uint32_t, sizeof(uint32_t),
                                    boost::endian::order::little>(
      reinterpret_cast<const unsigned char*>(p));
}

inline uint64_t ReadU64(const char* p) {
  return boost::endian::endian_load<uint64_t, sizeof(uint64_t),
                                    boost::endian::order::little>(
      reinterpret_cast<const unsigned char*>(p));
}

inline void StoreU32(char* p, uint32_t value) {
  boost::endian::endian_store<uint32_t, sizeof(uint32_t),
                              boost::endian::order::little>(
      reinterpret_cast<unsigned char*>(p), value);
}

inline void StoreU64(char* p, uint64_t value) {
  boost::endian::endian_store<uint64_t, sizeof(uint64_t),
                              boost::endian::order::little>(
      reinterpret_cast<unsigned char*>(p), value);
}

inline void WriteU32(std::ostream* out, uint32_t value) {
  std::array<char, sizeof(uint32_t)> bytes{};
  StoreU32(bytes.data(), value);
  out->write(bytes.data(), bytes.size());
}

inline void WriteU64(std::ostream* out, uint64_t value) {
  std::array<char, sizeof(uint64_t)> bytes{};
  StoreU64(bytes.data(), value);
  out->write(bytes.data(), bytes.size());
}

inline void WriteZeros(std::ostream* out, size_t count) {
  static constexpr std::array<char, 64> kZeros{};
  while (count > 0) {
    const size_t chunk = std::min(count, kZeros.size());
    out->write(kZeros.data(), chunk);
    count -= chunk;
  }
}

inline bool InRange(uint64_t offset, uint64_t size, uint64_t file_size) {
  return offset <= file_size && size <= file_size - offset;
}

inline bool FitsU32(size_t value) {
  return value <= std::numeric_limits<uint32_t>::max();
}

inline uint64_t AlignValue(uint64_t value, uint64_t alignment) {
  return boost::alignment::align_up(value, alignment);
}

inline path MakeTemporaryPath(const path& output_path) {
  path result = output_path;
  result += path(".tmp." +
                 boost::uuids::to_string(boost::uuids::random_generator()()));
  return result;
}

class TemporaryFileGuard {
 public:
  explicit TemporaryFileGuard(path file_path)
      : file_path_(std::move(file_path)) {}
  ~TemporaryFileGuard() {
    if (!committed_) {
      std::error_code ec;
      std::filesystem::remove(file_path_, ec);
    }
  }
  void Commit() { committed_ = true; }

 private:
  path file_path_;
  bool committed_ = false;
};

inline bool FlushFileDurably(const path& file_path) {
#ifdef _WIN32
  HANDLE handle =
      CreateFileW(file_path.wstring().c_str(), GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    return false;
  }
  const bool ok = FlushFileBuffers(handle) != 0;
  CloseHandle(handle);
  return ok;
#else
  const int fd = open(file_path.c_str(), O_RDONLY);
  if (fd < 0) {
    return false;
  }
  const bool ok = fsync(fd) == 0;
  close(fd);
  return ok;
#endif
}

inline bool FlushParentDirectory(const path& file_path) {
#ifdef _WIN32
  return true;
#else
  std::filesystem::path parent = file_path.parent_path();
  if (parent.empty()) {
    parent = ".";
  }
  int flags = O_RDONLY;
#ifdef O_DIRECTORY
  flags |= O_DIRECTORY;
#endif
  const int fd = open(parent.c_str(), flags);
  if (fd < 0) {
    return false;
  }
  const bool ok = fsync(fd) == 0;
  close(fd);
  return ok;
#endif
}

inline bool InstallAtomically(const path& temporary_path,
                              const path& output_path,
                              std::error_code* error) {
  if (!error) {
    return false;
  }
  error->clear();
  std::filesystem::rename(temporary_path, output_path, *error);
  if (!*error) {
    // The rename already committed the new file. Directory fsync is a
    // durability enhancement; failure must not report a reversible install
    // error after the replacement has happened.
    FlushParentDirectory(output_path);
    return true;
  }
#ifdef _WIN32
  if (MoveFileExW(temporary_path.wstring().c_str(),
                  output_path.wstring().c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    error->clear();
    return true;
  }
  *error =
      std::error_code(static_cast<int>(GetLastError()), std::system_category());
#endif
  return false;
}

}  // namespace rewrite_internal
}  // namespace rime

#endif  // RIME_REWRITE_BINARY_H_
