//
// Copyright RIME Developers
// Distributed under the BSD License
//

#include <rime/build_config.h>

#ifdef RIME_ENABLE_LOGGING

#include <rime/setup.h>

namespace rime {

RIME_DLL void SetLogHandler(RimeLogHandler /*handler*/,
                            void* /*context_object*/) {
  // google-glog owns logging when ENABLE_LOGGING is on.
}

}  // namespace rime

#else

#include "logging.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>

#include <rime/setup.h>

namespace rime {

namespace {

struct LogHandlerRegistration {
  RimeLogHandler handler = nullptr;
  void* context_object = nullptr;
};

std::shared_ptr<const LogHandlerRegistration> g_registration;
std::mutex g_registration_mutex;

std::shared_ptr<const LogHandlerRegistration> current_registration() {
  return std::atomic_load_explicit(&g_registration, std::memory_order_acquire);
}

unsigned long long current_thread_id() {
  return static_cast<unsigned long long>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

}  // namespace

void SetLogHandler(RimeLogHandler handler, void* context_object) {
  std::lock_guard<std::mutex> lock(g_registration_mutex);
  if (!handler) {
    std::atomic_store_explicit(&g_registration,
                               std::shared_ptr<const LogHandlerRegistration>(),
                               std::memory_order_release);
    return;
  }
  auto registration = std::make_shared<LogHandlerRegistration>();
  registration->handler = handler;
  registration->context_object = context_object;
  std::shared_ptr<const LogHandlerRegistration> stored =
      std::move(registration);
  std::atomic_store_explicit(&g_registration, std::move(stored),
                             std::memory_order_release);
}

namespace logging_internal {

bool log_is_enabled(LogSeverity severity) {
  if (severity == LogSeverity::kFatal) {
    return true;
  }
  return current_registration() != nullptr;
}

std::ostream& null_stream() {
  class NullBuffer : public std::streambuf {
   protected:
    int overflow(int c) override { return c; }
  };
  static thread_local NullBuffer buffer;
  static thread_local std::ostream stream(&buffer);
  return stream;
}

void write_log(const char* file,
               int line,
               const char* function,
               LogSeverity severity,
               std::string message) {
  auto registration = current_registration();
  if (registration && registration->handler) {
    RimeLogRecord record;
    RIME_STRUCT_INIT(RimeLogRecord, record);
    record.level = static_cast<int>(severity);
    record.message = message.c_str();
    record.file = file;
    record.line = line;
    record.function = function;
    record.thread_id = current_thread_id();
    registration->handler(registration->context_object, &record);
  } else if (severity == LogSeverity::kFatal) {
    std::fprintf(stderr, "[FATAL] %s:%d: %s\n", file ? file : "", line,
                 message.c_str());
    std::fflush(stderr);
  }

  if (severity == LogSeverity::kFatal) {
    std::abort();
  }
}

[[noreturn]] void check_failed(const char* file,
                               int line,
                               const char* expression) {
  write_log(file, line, "CHECK", LogSeverity::kFatal,
            std::string("Check failed: ") + expression);
  std::abort();
}

[[noreturn]] void check_op_failed(const char* file,
                                  int line,
                                  const char* expression,
                                  const std::string& value1,
                                  const std::string& value2) {
  write_log(file, line, "CHECK", LogSeverity::kFatal,
            std::string("Check failed: ") + expression + " (" + value1 +
                " vs " + value2 + ")");
  std::abort();
}

}  // namespace logging_internal

}  // namespace rime

#endif  // RIME_ENABLE_LOGGING
