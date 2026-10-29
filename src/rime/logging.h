//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_LOGGING_H_
#define RIME_LOGGING_H_

#include <ostream>
#include <sstream>
#include <string>

#include "rime_api.h"

namespace rime {

enum class LogSeverity : int {
  kInfo = RIME_LOG_INFO,
  kWarning = RIME_LOG_WARNING,
  kError = RIME_LOG_ERROR,
  kFatal = RIME_LOG_FATAL,
};

namespace logging_internal {

//! Returns true when a record with the given severity should be emitted.
RIME_DLL bool log_is_enabled(LogSeverity severity);

//! Emits a record to the installed handler. FATAL records always abort.
RIME_DLL void write_log(const char* file,
                        int line,
                        LogSeverity severity,
                        std::string message);

class LogMessage {
 public:
  LogMessage(const char* file, int line, LogSeverity severity)
      : file_(file), line_(line), severity_(severity) {}

  ~LogMessage() { write_log(file_, line_, severity_, stream_.str()); }

  std::ostream& stream() { return stream_; }

 private:
  const char* file_;
  int line_;
  LogSeverity severity_;
  std::ostringstream stream_;
};

}  // namespace logging_internal

}  // namespace rime

// A statement-shaped no-op that still type-checks a streamed log statement
// without evaluating the value.
#define RIME_LOG_NOOP                                              \
  for (bool rime_log_disabled = false; rime_log_disabled;          \
       rime_log_disabled = false)                                  \
  ::rime::logging_internal::LogMessage(__FILE__, __LINE__,         \
                                       ::rime::LogSeverity::kInfo) \
      .stream()

// Severity tokens are pasted into these level macros so that Windows macros
// such as ERROR from wingdi.h are not expanded in LOG(ERROR).
#define RIME_LOG_LEVEL_INFO RIME_LOG_INFO
#define RIME_LOG_LEVEL_WARNING RIME_LOG_WARNING
#define RIME_LOG_LEVEL_ERROR RIME_LOG_ERROR
#define RIME_LOG_LEVEL_FATAL RIME_LOG_FATAL

// The condition is evaluated once. When it is false, the loop body is skipped,
// so the streamed arguments are not evaluated either.
#define RIME_LOG_STREAM(level, condition)                               \
  for (bool rime_log_enabled =                                          \
           (condition) && ::rime::logging_internal::log_is_enabled(     \
                              static_cast<::rime::LogSeverity>(level)); \
       rime_log_enabled; rime_log_enabled = false)                      \
  ::rime::logging_internal::LogMessage(                                 \
      __FILE__, __LINE__, static_cast<::rime::LogSeverity>(level))      \
      .stream()

#define LOG(severity) RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, true)
#define LOG_IF(severity, condition) \
  RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, (condition))

// CHECK is librime's only fatal assertion. Unlike DCHECK it is active in all
// build configurations and aborts the process when the condition fails.
#define CHECK(condition)                              \
  RIME_LOG_STREAM(RIME_LOG_LEVEL_FATAL, !(condition)) \
      << "Check failed: " #condition

#ifndef NDEBUG
#define DLOG(severity) RIME_LOG_STREAM(RIME_LOG_LEVEL_##severity, true)
#define DCHECK(condition) CHECK(condition)
#else
#define DLOG(severity) RIME_LOG_NOOP
#define DCHECK(condition) RIME_LOG_NOOP
#endif  // NDEBUG

#endif  // RIME_LOGGING_H_
