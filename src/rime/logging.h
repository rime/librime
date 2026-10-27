//
// Copyright RIME Developers
// Distributed under the BSD License
//
#ifndef RIME_LOGGING_H_
#define RIME_LOGGING_H_

#include <rime_api.h>

#include <atomic>
#include <cctype>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

// This header implements the subset of glog's logging surface that librime
// uses, backed by spdlog.
//
// Logging behavior:
//   * Four per-severity files per process, mode 0600:
//       <app>.<YYYYmmdd>.<pid>.INFO.log
//       <app>.<YYYYmmdd>.<pid>.WARNING.log
//       <app>.<YYYYmmdd>.<pid>.ERROR.log
//       <app>.<YYYYmmdd>.<pid>.FATAL.log
//     As in glog, a message is written to its own severity file and to every
//     lower-severity file: INFO receives all messages, WARNING receives
//     WARNING and above, ERROR receives ERROR and above, and FATAL receives
//     only FATAL messages.
//   * All four files and links are created when logging is initialized.
//   * <app>.INFO, <app>.WARNING, <app>.ERROR and <app>.FATAL link to the
//     corresponding current file.
//   * Lines use spdlog's default full pattern, e.g.
//       [2026-10-23 12:34:56.123] [app] [info] [file.cc:42] message
//   * With a log file open, ERROR and FATAL messages are mirrored to stderr
//     (glog's default --stderrthreshold).  INFO and WARNING go to stderr only
//     when no file is available, when the log directory is "", or when
//     ALSO_LOG_TO_STDERR is enabled at build time.
//   * VLOG is disabled unless SetVlogLevel() is called.
namespace rime {

enum class LogSeverity { Info, Warning, Error, Fatal };

// A log message is assembled with operator<< and written to the logging
// backend in the destructor, so that LOG(INFO) << ... stays source-compatible
// with glog.
class RIME_DLL LogMessage {
 public:
  LogMessage(LogSeverity severity, const char* file, int line);
  ~LogMessage();

  LogMessage(const LogMessage&) = delete;
  LogMessage& operator=(const LogMessage&) = delete;

  std::ostream& stream() { return stream_; }

 private:
  LogSeverity severity_;
  const char* file_;
  int line_;
  std::ostringstream stream_;
};

// Helper for the LOG_IF family.  It lets `LOG_IF(...) << value` compile while
// the void-returning operator& keeps the whole expression well-formed.
class LogMessageVoidify {
 public:
  LogMessageVoidify() = default;
  void operator&(std::ostream&) {}
};

// Initializes the process-wide logger.  Calling this more than once is
// ignored after the first call.
RIME_DLL void InitializeLogging(const char* app_name,
                                int min_log_level,
                                const char* log_dir);
RIME_DLL void ShutdownLogging();

RIME_DLL bool IsLoggingInitialized();
// True when messages are not written to a log file and stderr is the primary
// destination.  When logging to a file, ERROR and FATAL messages are still
// mirrored to stderr, but this returns false.
RIME_DLL bool IsLogToStderr();
// Directories that may contain log files.  Empty when logging to stderr only.
RIME_DLL std::vector<std::string> GetLoggingDirectories();
RIME_DLL bool VlogIsOn(int verbose_level);
// Enables VLOG(n) messages up to the given level.  Verbose logging is
// process-wide and disabled by default; a non-positive level disables it.
RIME_DLL void SetVlogLevel(int verbose_level);

// Writes an already-formatted message.  Exposed for LogMessage and tests.
RIME_DLL void LogMessageToSink(LogSeverity severity,
                               const char* file,
                               int line,
                               const std::string& message);

template <typename T>
T CheckNotNull(T value, const char* file, int line, const char* expression) {
  if (value == nullptr) {
    LogMessage(LogSeverity::Fatal, file, line).stream()
        << "Check failed: " << expression << " is NULL";
  }
  return value;
}

// Release-mode DCHECK_NOTNULL keeps evaluating and returning its argument, but
// does not emit a check.  The function call also avoids -Wunused-value when
// the result is intentionally ignored.
template <typename T>
T DCheckNotNull(T value) {
  return value;
}

inline bool CStringEquals(const char* lhs,
                          const char* rhs,
                          bool case_sensitive) {
  if (lhs == nullptr || rhs == nullptr)
    return lhs == rhs;
  if (case_sensitive)
    return std::strcmp(lhs, rhs) == 0;
  while (*lhs != '\0' && *rhs != '\0') {
    if (std::tolower(static_cast<unsigned char>(*lhs)) !=
        std::tolower(static_cast<unsigned char>(*rhs)))
      return false;
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

}  // namespace rime

#define RIME_LOG_SEVERITY_INFO ::rime::LogSeverity::Info
#define RIME_LOG_SEVERITY_WARNING ::rime::LogSeverity::Warning
#define RIME_LOG_SEVERITY_ERROR ::rime::LogSeverity::Error
#define RIME_LOG_SEVERITY_FATAL ::rime::LogSeverity::Fatal
#ifdef NDEBUG
#define RIME_LOG_SEVERITY_DFATAL ::rime::LogSeverity::Error
#else
#define RIME_LOG_SEVERITY_DFATAL ::rime::LogSeverity::Fatal
#endif

// Every macro taking a severity pastes it exactly once.  Forwarding the
// severity through another macro would expand Windows' ERROR macro first and
// turn e.g. LOG_IF(ERROR, ...) into LOG_IF(0L, ...), so each macro below
// contains RIME_LOG_SEVERITY_##severity directly.
#define RIME_LOG_MESSAGE(severity, file, line) \
  ::rime::LogMessage(severity, file, line).stream()
#define LOG(severity) \
  RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, __LINE__)

#define LOG_IF(severity, condition)                                     \
  static_cast<void>(0),                                                 \
      !(condition) ? (void)0                                            \
                   : ::rime::LogMessageVoidify() &                      \
                         RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, \
                                          __FILE__, __LINE__)

#define LOG_ASSERT(condition) \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition

#define RIME_LOG_JOIN_IMPL(lhs, rhs) lhs##rhs
#define RIME_LOG_JOIN(lhs, rhs) RIME_LOG_JOIN_IMPL(lhs, rhs)

#define LOG_EVERY_N(severity, n)                                           \
  static ::std::atomic<int> RIME_LOG_JOIN(rime_log_counter_, __LINE__)(0); \
  if (++RIME_LOG_JOIN(rime_log_counter_, __LINE__) % (n) == 1)             \
  RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, __LINE__)

#define LOG_IF_EVERY_N(severity, condition, n)                                \
  static ::std::atomic<int> RIME_LOG_JOIN(rime_log_counter_, __LINE__)(0);    \
  ++RIME_LOG_JOIN(rime_log_counter_, __LINE__);                               \
  if ((condition) && (RIME_LOG_JOIN(rime_log_counter_, __LINE__) % (n) == 1)) \
  RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, __LINE__)

#define VLOG_IS_ON(verboselevel) ::rime::VlogIsOn(verboselevel)
#define VLOG(verboselevel) LOG_IF(INFO, VLOG_IS_ON(verboselevel))

#define CHECK(condition) \
  LOG_IF(FATAL, !(condition)) << "Check failed: " #condition " "
#define CHECK_EQ(val1, val2) \
  LOG_IF(FATAL, !((val1) == (val2))) << "Check failed: " #val1 " == " #val2 " "
#define CHECK_NE(val1, val2) \
  LOG_IF(FATAL, !((val1) != (val2))) << "Check failed: " #val1 " != " #val2 " "
#define CHECK_LE(val1, val2) \
  LOG_IF(FATAL, !((val1) <= (val2))) << "Check failed: " #val1 " <= " #val2 " "
#define CHECK_LT(val1, val2) \
  LOG_IF(FATAL, !((val1) < (val2))) << "Check failed: " #val1 " < " #val2 " "
#define CHECK_GE(val1, val2) \
  LOG_IF(FATAL, !((val1) >= (val2))) << "Check failed: " #val1 " >= " #val2 " "
#define CHECK_GT(val1, val2) \
  LOG_IF(FATAL, !((val1) > (val2))) << "Check failed: " #val1 " > " #val2 " "

#define CHECK_NOTNULL(val) ::rime::CheckNotNull((val), __FILE__, __LINE__, #val)

#define CHECK_STREQ(str1, str2)                               \
  LOG_IF(FATAL, !::rime::CStringEquals((str1), (str2), true)) \
      << "Check failed: " #str1 " == " #str2 " "
#define CHECK_STRCASEEQ(str1, str2)                            \
  LOG_IF(FATAL, !::rime::CStringEquals((str1), (str2), false)) \
      << "Check failed: " #str1 " == " #str2 " (case-insensitive) "
#define CHECK_STRNE(str1, str2)                              \
  LOG_IF(FATAL, ::rime::CStringEquals((str1), (str2), true)) \
      << "Check failed: " #str1 " != " #str2 " "
#define CHECK_STRCASENE(str1, str2)                           \
  LOG_IF(FATAL, ::rime::CStringEquals((str1), (str2), false)) \
      << "Check failed: " #str1 " != " #str2 " (case-insensitive) "

#ifdef NDEBUG
#define DLOG(severity)                                                    \
  static_cast<void>(0),                                                   \
      true ? (void)0                                                      \
           : ::rime::LogMessageVoidify() &                                \
                 RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, \
                                  __LINE__)
#define DVLOG(verboselevel) \
  static_cast<void>(0), true ? (void)0 : ::rime::LogMessageVoidify() & LOG(INFO)
#define DLOG_IF(severity, condition)                                     \
  static_cast<void>(0),                                                  \
      (true || !(condition))                                             \
          ? (void)0                                                      \
          : ::rime::LogMessageVoidify() &                                \
                RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, \
                                 __LINE__)
#define DLOG_EVERY_N(severity, n)                                         \
  static_cast<void>(0),                                                   \
      true ? (void)0                                                      \
           : ::rime::LogMessageVoidify() &                                \
                 RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, \
                                  __LINE__)
#define DLOG_IF_EVERY_N(severity, condition, n)                          \
  static_cast<void>(0),                                                  \
      (true || !(condition))                                             \
          ? (void)0                                                      \
          : ::rime::LogMessageVoidify() &                                \
                RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, \
                                 __LINE__)
#define DLOG_ASSERT(condition) \
  static_cast<void>(0), true ? (void)0 : ::rime::LogMessageVoidify() & LOG(INFO)
#define DCHECK(condition) \
  while (false)           \
  CHECK(condition)
#define DCHECK_EQ(val1, val2) \
  while (false)               \
  CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) \
  while (false)               \
  CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) \
  while (false)               \
  CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) \
  while (false)               \
  CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) \
  while (false)               \
  CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) \
  while (false)               \
  CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) ::rime::DCheckNotNull((val))
#define DCHECK_STREQ(str1, str2) \
  while (false)                  \
  CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) \
  while (false)                      \
  CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) \
  while (false)                  \
  CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) \
  while (false)                      \
  CHECK_STRCASENE(str1, str2)
#else
#define DLOG(severity) \
  RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, __LINE__)
#define DVLOG(verboselevel) VLOG(verboselevel)
#define DLOG_IF(severity, condition)                                    \
  static_cast<void>(0),                                                 \
      !(condition) ? (void)0                                            \
                   : ::rime::LogMessageVoidify() &                      \
                         RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, \
                                          __FILE__, __LINE__)
#define DLOG_EVERY_N(severity, n)                                          \
  static ::std::atomic<int> RIME_LOG_JOIN(rime_log_counter_, __LINE__)(0); \
  if (++RIME_LOG_JOIN(rime_log_counter_, __LINE__) % (n) == 1)             \
  RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, __LINE__)
#define DLOG_IF_EVERY_N(severity, condition, n)                               \
  static ::std::atomic<int> RIME_LOG_JOIN(rime_log_counter_, __LINE__)(0);    \
  ++RIME_LOG_JOIN(rime_log_counter_, __LINE__);                               \
  if ((condition) && (RIME_LOG_JOIN(rime_log_counter_, __LINE__) % (n) == 1)) \
  RIME_LOG_MESSAGE(RIME_LOG_SEVERITY_##severity, __FILE__, __LINE__)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)
#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_NOTNULL(val) CHECK_NOTNULL(val)
#define DCHECK_STREQ(str1, str2) CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) CHECK_STRCASENE(str1, str2)
#endif

#endif  // RIME_LOGGING_H_
