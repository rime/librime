//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <rime/build_config.h>

#ifdef RIME_ENABLE_LOGGING

#include <cstdio>
#include <mutex>

#include <rime/setup.h>

namespace rime {

namespace {

void WarnLogHandlerUnsupported() {
  std::fprintf(stderr,
               "librime: set_log_handler is unavailable in this build "
               "(ENABLE_LOGGING=ON); use google-glog or rebuild with "
               "ENABLE_LOGGING=OFF.\n");
}

}  // namespace

bool SetLogHandler(LogHandler handler) {
  if (handler) {
    static std::once_flag warned;
    std::call_once(warned, WarnLogHandlerUnsupported);
  }
  return false;
}

bool ClearLogHandler() {
  return false;
}

}  // namespace rime

#else

#include "logging.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <utility>

#include <rime/setup.h>

namespace rime {

namespace {

// The handler is protected by g_handler_mutex. Calling it while holding the
// mutex serializes callbacks and makes SetLogHandler/ClearLogHandler wait for
// in-flight callbacks before returning.
LogHandler g_handler;
std::mutex g_handler_mutex;

// Fast path for log statements while no handler is installed. Kept in sync
// with g_handler under g_handler_mutex, so disabled logging stays lock-free.
std::atomic<bool> g_handler_installed{false};

unsigned long long current_thread_id() {
  return static_cast<unsigned long long>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

}  // namespace

bool SetLogHandler(LogHandler handler) {
  std::lock_guard<std::mutex> lock(g_handler_mutex);
  g_handler = std::move(handler);
  g_handler_installed.store(static_cast<bool>(g_handler));
  return true;
}

bool ClearLogHandler() {
  return SetLogHandler(LogHandler());
}

namespace logging_internal {

bool log_is_enabled(LogSeverity severity) {
  return severity == LogSeverity::kFatal || g_handler_installed.load();
}

void write_log(const char* file,
               int line,
               LogSeverity severity,
               std::string message) {
  std::lock_guard<std::mutex> lock(g_handler_mutex);
  if (g_handler) {
    RimeLogRecord record;
    RIME_STRUCT_INIT(RimeLogRecord, record);
    record.level = static_cast<int>(severity);
    record.message = message.c_str();
    record.file = file;
    record.line = line;
    record.thread_id = current_thread_id();
    try {
      g_handler(record);
    } catch (...) {
      // Logging must not propagate handler failures into the application.
    }
  } else if (severity == LogSeverity::kFatal) {
    std::fprintf(stderr, "[FATAL] %s:%d: %s\n", file ? file : "", line,
                 message.c_str());
    std::fflush(stderr);
  }

  if (severity == LogSeverity::kFatal) {
    std::abort();
  }
}

}  // namespace logging_internal

}  // namespace rime

#endif  // RIME_ENABLE_LOGGING
